#include "compiler/types/special.hpp"
#include "compiler/llvm_helper.hpp"

#include "shading/distribution.hpp"
#include "geometry/ray.hpp"

#include "compiler/type_conversion.hpp"

using namespace raytrace;
using namespace llvm;
using namespace std;

//Ray

ray_type::ray_type(type_table *types) :
  type(types, "ray", "r"),
  type_value(compute_type())
{
  
}

Type *ray_type::compute_type() {
  Type *v3 = StructType::get(getGlobalContext(),
			     ArrayRef<Type*>(vector<Type*>(3, Type::getFloatTy(getGlobalContext()))),
			     true);
  vector<Type*> elems{v3, v3, Type::getFloatTy(getGlobalContext()), Type::getFloatTy(getGlobalContext())};
  return StructType::create(getGlobalContext(), ArrayRef<Type*>(elems), "ray", true);
}

Type *ray_type::llvm_type() const {
  return type_value;
}

typed_value_container ray_type::create(Module *module, IRBuilder<> &builder, typed_value_vector &args,
				       const type_conversion_table &conversions) const {
  return errors::codegen_call<typed_value_vector, typed_value_container>(args, [&] (vector<typed_value> &ctor_args) -> typed_value_container {
      if (ctor_args.size() != 4) {
	stringstream err_ss;
	err_ss << " Ray constructor expects 4 arguments, received " << ctor_args.size();
	return errors::make_error<errors::error_message>(err_ss.str(), 0, 0);
      }

      vector<type_spec> arg_types;
      for (auto arg_it = ctor_args.begin(); arg_it != ctor_args.end(); ++arg_it) {
	arg_types.push_back(arg_it->get<1>());
      }
      
      //check argument types
      bool valid_arguments = true;
      int unused;
      if (!conversions.can_convert(arg_types[0], types->at("vec3"), unused, unused)) valid_arguments = false;
      if (!conversions.can_convert(arg_types[1], types->at("vec3"), unused, unused)) valid_arguments = false;
      if (!conversions.can_convert(arg_types[2], types->at("float"), unused, unused)) valid_arguments = false;
      if (!conversions.can_convert(arg_types[3], types->at("float"), unused, unused)) valid_arguments = false;
      if (!valid_arguments) {
	return errors::make_error<errors::error_message>("Invalid arguments. Expected (vec3, vec3, float, float).", 0, 0);
      }
      
      
      code_value origin = (arg_types[0] == types->at("vec3")) ?
	ctor_args[0].get<0>().extract_value() :
	conversions.convert(arg_types[0], ctor_args[0].get<0>().extract_value(),
			    types->at("vec3"), module, builder);
      
      code_value direction = conversions.convert(arg_types[1], ctor_args[1].get<0>().extract_value(),
						 types->at("vec3"), module, builder);
      code_value min_t = conversions.convert(arg_types[2], ctor_args[2].get<0>().extract_value(),
					     types->at("float"), module, builder);
      code_value max_t = conversions.convert(arg_types[3], ctor_args[3].get<0>().extract_value(),
					     types->at("float"), module, builder);
      boost::function<typed_value_container (value &, value &,
					     value &, value &)> ctor = [this, module, &builder] (value &origin,
												 value &direction,
												 value &min_t, value &max_t) -> typed_value_container {
	Value *ray_val = UndefValue::get(type_value);
	Value *o_val = origin.extract_value();
	Value *d_val = direction.extract_value();
	
	for (unsigned int i = 0; i < 3; ++i) {
	  vector<unsigned int> o_idx{0, i};
	  vector<unsigned int> d_idx{1, i};
	  
	  ray_val = builder.CreateInsertValue(ray_val,
					      builder.CreateExtractValue(o_val, ArrayRef<unsigned int>(i)),
					      ArrayRef<unsigned int>(o_idx));
	  ray_val = builder.CreateInsertValue(ray_val,
					      builder.CreateExtractValue(d_val, ArrayRef<unsigned int>(i)),
					      ArrayRef<unsigned int>(d_idx));
	}
	
	ray_val = builder.CreateInsertValue(ray_val, min_t.extract_value(), ArrayRef<unsigned int>(2));
	ray_val = builder.CreateInsertValue(ray_val, max_t.extract_value(), ArrayRef<unsigned int>(3));
	return typed_value(ray_val, types->at("ray"));
      };
      
      return errors::codegen_apply(ctor, origin, direction, min_t, max_t);
    });
}

//Intersection

Type *intersection_type::llvm_type() const {
  Type *byte_type = Type::getInt8Ty(getGlobalContext());
  return ArrayType::get(byte_type, sizeof(intersection));
}

//Light Source

Type *light_type::llvm_type() const {
  return Type::getInt32PtrTy(getGlobalContext());
}

//Distribution Object

dfunc_type::dfunc_type(type_table *types) :
  type(types, "dfunc", "dist")
{
  
}

Type *dfunc_type::llvm_type() const {
  Type *byte_type = Type::getInt8Ty(getGlobalContext());
  return ArrayType::get(byte_type, sizeof(shade_tree::node_ptr));
}

typed_value_container dfunc_type::initialize(Module *, IRBuilder<> &) const {
  return errors::make_error<errors::error_message>("No default initialization for distributions.", 0, 0);
}

Value *dfunc_type::copy(Value *value, Module *module, IRBuilder<> &builder) {
  Type *pointer_type = llvm_type()->getPointerTo();
  vector<Type*> arg_type({pointer_type, pointer_type});
  FunctionType *ty = FunctionType::get(Type::getVoidTy(getGlobalContext()), arg_type, false);
  Function *copy_f = cast<Function>(module->getOrInsertFunction("gd_builtin_copy_dfunc", ty));

  Value *val_ptr = CreateEntryBlockAlloca(builder, llvm_type(), "dfunc_src");
  builder.CreateStore(value, val_ptr, false);

  Value *copy = CreateEntryBlockAlloca(builder, llvm_type(), "dfunc_dst");
  builder.CreateCall2(copy_f, val_ptr, copy);
  return builder.CreateLoad(copy);
}

codegen_void dfunc_type::destroy(Value *value, Module *module, IRBuilder<> &builder) {
  vector<Type*> arg_type({llvm_type()->getPointerTo()});
  FunctionType *ty = FunctionType::get(Type::getVoidTy(getGlobalContext()), arg_type, false);
  Function *dtor = cast<Function>(module->getOrInsertFunction("gd_builtin_destroy_dfunc", ty));

  builder.CreateCall(dtor, value);
  return empty_type();
}

typed_value_container dfunc_type::create(Module *module, IRBuilder<> &builder,
					 typed_value_vector &args,
					 const type_conversion_table &conversions) const {
  boost::function<typed_value_container (vector<typed_value>&)> ctor = [this, module, &builder] (vector<typed_value> &args) -> typed_value_container {
    vector<Value*> shader_args;
    
    //ensure the argument types are {shader_handle, ray, vec2, isect}.
    vector<type_spec> expected_args({types->at("shader_handle"), types->at("ray"), types->at("vec2"), types->at("isect")});

    if (args.size() != 4) {
      stringstream err_ss;
      err_ss << "dfunc constructor expected 4 arguments, received " << args.size();
      return errors::make_error<errors::error_message>(err_ss.str(), 0, 0);
    }
    for (unsigned int i = 0; i < args.size(); ++i) {
      type_spec arg_ty = args[i].get<1>();
      if (*arg_ty != *expected_args[i]) {
	stringstream err_ss;
	err_ss << "Error in argument " << i << ": Expected type '" << expected_args[i]->name << "' found '" << arg_ty->name << "'";
	return errors::make_error<errors::error_message>(err_ss.str(), 0, 0);
      }
    }
    
    auto arg_it = args.begin();
    Value *shader_func = arg_it->get<0>().extract_value();
    ++arg_it;

    while (arg_it != args.end()) {
      shader_args.push_back(arg_it->get<0>().extract_value());
      ++arg_it;
    }

    return typed_value(builder.CreateCall(shader_func, shader_args), types->at("dfunc"));
  };

  return errors::codegen_call<typed_value_vector, typed_value_container>(args, ctor);
}

//Shader Flag

Type *shader_flag_type::llvm_type() const {
  return Type::getInt64Ty(getGlobalContext());
}

typed_value_container shader_flag_type::create(Module *module, IRBuilder<> &builder,
					       typed_value_vector &args, const type_conversion_table &conversions) const {
  auto build = [this, &conversions, module, &builder] (vector<typed_value> &args) -> typed_value_container {
    if (args.size() != 1) {
      stringstream err_ss;
      err_ss << name << " constructor expects one integer argument, received " << args.size();
      return errors::make_error<errors::error_message>(err_ss.str(), 0, 0);
    }
    
    typed_value &arg = args[0];
    int unused;
    if (!conversions.can_convert(arg.get<1>(), types->at("int"), unused, unused)) {
      return errors::make_error<errors::error_message>("Argument must be convertible to an integer", 0, 0);
    }
    
    //TODO: Check if this is between 1 and 64
    code_value bit_idx = conversions.convert(arg.get<1>(), arg.get<0>().extract_value(),
					     types->at("int"),
					     module, builder);
    return errors::codegen_call<code_value, typed_value_container>(bit_idx,
								   [this, &builder] (value &v) -> typed_value_container {
								     return typed_value(builder.CreateShl(ConstantInt::get(getGlobalContext(), APInt(64, 1, false)),
													  v.extract_value(), "shifted_flag"),
											types->at("shader_flag"));
								   });
    
  };

  return errors::codegen_call<typed_value_vector,
			      typed_value_container>(args, build);
}

codegen_constant shader_flag_type::create_const(Module *module, IRBuilder<> &builder,
						codegen_const_vector &args, const type_conversion_table &conversions) const {
  auto build = [this, &conversions, module, &builder] (vector<typed_constant> &args) -> codegen_constant {
    if (args.size() != 1) {
      stringstream err_ss;
      err_ss << name << " constructor expects one integer argument, received " << args.size();
      return errors::make_error<errors::error_message>(err_ss.str(), 0, 0);
    }
    
    typed_constant &arg = args[0];
    if (arg.get<1>() != types->at("int")) {
      return errors::make_error<errors::error_message>("Argument must be convertible to an integer", 0, 0);
    }

    uint64_t bit_idx = cast<ConstantInt>(arg.get<0>())->getZExtValue();
    if (bit_idx > 64) {
      return errors::make_error<errors::error_message>("Argument must be an integer in [0, 64]", 0, 0);
    }

    if (bit_idx == 0) return typed_constant(ConstantInt::get(getGlobalContext(), APInt(64, 0, false)), types->at("shader_flag"));

    uint64_t flag_val = 1 << (bit_idx - 1);
    return typed_constant(ConstantInt::get(getGlobalContext(), APInt(64, flag_val, false)), types->at("shader_flag"));
  };
  return errors::codegen_call<codegen_const_vector,
			      codegen_constant>(args, build);
}

typed_value_container shader_flag_type::initialize(Module *module, IRBuilder<> &builder) const {
  return typed_value(ConstantInt::get(getGlobalContext(), APInt(64, 0, false)),
		     types->at("shader_flag"));
}

//Shader Handle

Type *shader_handle_type::llvm_type() const {
  vector<Type*> arg_ty({types->at("ray")->llvm_type(), types->at("vec2")->llvm_type(), types->at("isect")->llvm_type()});
  FunctionType *ft = FunctionType::get(types->at("dfunc")->llvm_type(), arg_ty, false);
  return ft->getPointerTo();
}

//Context Pointer

Type *context_ptr_type::llvm_type() const {
  return Type::getInt32PtrTy(getGlobalContext());
}
