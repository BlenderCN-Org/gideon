#include "compiler/types/primitive.hpp"
#include "compiler/operations.hpp"
#include "compiler/llvm_helper.hpp"

using namespace raytrace;
using namespace llvm;
using namespace std;

Type *void_type::llvm_type() const {
  return Type::getVoidTy(getGlobalContext());
}

Type *bool_type::llvm_type() const {
  return Type::getInt1Ty(getGlobalContext());
}

//Ints

Type *int_type::llvm_type() const {
  return Type::getInt32Ty(getGlobalContext());
}

codegen_value int_type::op_add(Module *module, IRBuilder<> &builder,
			       codegen_value &lhs, codegen_value &rhs) const {
  typedef errors::argument_value_join<codegen_value, codegen_value>::result_value_type arg_val_type;
  boost::function<codegen_value (arg_val_type &)> op = [&builder, module] (arg_val_type &val) {
    return builder.CreateAdd(val.get<0>(), val.get<1>(), "i_add_tmp");
  };
  return errors::codegen_call_args(op, lhs, rhs);
}

codegen_value int_type::op_less(Module *module, IRBuilder<> &builder,
				  codegen_value &lhs, codegen_value &rhs) const {
  typedef errors::argument_value_join<codegen_value, codegen_value>::result_value_type arg_val_type;
  boost::function<codegen_value (arg_val_type &)> op = [&builder, module] (arg_val_type &val) {
    return builder.CreateICmpSLT(val.get<0>(), val.get<1>(), "i_lt_tmp");
  };
  return errors::codegen_call_args(op, lhs, rhs);
}

//Floats

Type *float_type::llvm_type() const {
  return Type::getFloatTy(getGlobalContext());
}

codegen_value float_type::op_add(Module *module, IRBuilder<> &builder,
				 codegen_value &lhs, codegen_value &rhs) const {
  typedef errors::argument_value_join<codegen_value, codegen_value>::result_value_type arg_val_type;
  boost::function<codegen_value (arg_val_type &)> op = [&builder, module] (arg_val_type &val) {
    return builder.CreateFAdd(val.get<0>(), val.get<1>(), "f_add_tmp");
  };
  return errors::codegen_call_args(op, lhs, rhs);
}

codegen_value float_type::op_less(Module *module, IRBuilder<> &builder,
				  codegen_value &lhs, codegen_value &rhs) const {
  typedef errors::argument_value_join<codegen_value, codegen_value>::result_value_type arg_val_type;
  boost::function<codegen_value (arg_val_type &)> op = [&builder, module] (arg_val_type &val) {
    return builder.CreateFCmpOLT(val.get<0>(), val.get<1>(), "f_lt_tmp");
  };
  return errors::codegen_call_args(op, lhs, rhs);
}

//String

string_type::string_type(type_table *types) :
  type(types, "string", "s"),
  str_type_value(StructType::create(getGlobalContext(),
				    ArrayRef<Type*>(vector<Type*>{
					Type::getInt1Ty(getGlobalContext()),
					  Type::getInt8PtrTy(getGlobalContext())}),
				    "string", true))
{

}

Type *string_type::llvm_type() const { return str_type_value; }

codegen_value string_type::initialize(Module *module, IRBuilder<> &builder) const {
  return builder.CreateInsertValue(UndefValue::get(llvm_type()),
				   ConstantInt::get(getGlobalContext(), APInt(1, true, true)), ArrayRef<unsigned int>(0), "init_str");
}

Value *string_type::copy(Value *value, Module *module, IRBuilder<> &builder) {
  Value *src_is_const = builder.CreateExtractValue(value, ArrayRef<unsigned int>(0), "str_is_const");
  Value *src_ptr = builder.CreateExtractValue(value, ArrayRef<unsigned int>(1), "str_data");
  
  Value *dst = CreateEntryBlockAlloca(builder, llvm_type(), "dst_str");
  Value *dst_is_const = builder.CreateStructGEP(dst, 0, "str_is_const");
  Value *dst_ptr = builder.CreateStructGEP(dst, 1, "str_data");

  Type *bool_type = Type::getInt1Ty(getGlobalContext());
  Type *ptr_type = Type::getInt8PtrTy(getGlobalContext());
  vector<Type*> arg_ty{bool_type, ptr_type, bool_type->getPointerTo(), ptr_type->getPointerTo()};

  FunctionType *ft = FunctionType::get(Type::getVoidTy(getGlobalContext()),
				       ArrayRef<Type*>(arg_ty), false);
  Function *copy = cast<Function>(module->getOrInsertFunction("gd_builtin_copy_string", ft));

  builder.CreateCall(copy, vector<Value*>{src_is_const, src_ptr, dst_is_const, dst_ptr});
  return builder.CreateLoad(dst, "str_copy");
}

codegen_void string_type::destroy(Value *value, Module *module, IRBuilder<> &builder) {
  Value *str = builder.CreateLoad(value, "str_ref");
  vector<Type*> arg_ty{Type::getInt1Ty(getGlobalContext()), Type::getInt8PtrTy(getGlobalContext())};
  FunctionType *ft = FunctionType::get(Type::getVoidTy(getGlobalContext()),
				       ArrayRef<Type*>(arg_ty), false);
  Function *dtor = cast<Function>(module->getOrInsertFunction("gd_builtin_destroy_string", ft));
  Value *is_const = builder.CreateExtractValue(str, ArrayRef<unsigned int>(0), "str_is_const");
  Value *str_ptr = builder.CreateExtractValue(str, ArrayRef<unsigned int>(1), "str_data");
  builder.CreateCall(dtor, vector<Value*>{is_const, str_ptr});
  
  return nullptr;
}

//Float 4

float4_type::float4_type(type_table *types) :
  type(types, "vec4", "v4", true),
  type_value(StructType::create(getGlobalContext(),
				ArrayRef<Type*>(vector<Type*>(4, Type::getFloatTy(getGlobalContext()))),
				"vec4", true))
{
  
}

Type *float4_type::llvm_type() const {
  return type_value;
}

codegen_value float4_type::create(Module *module, IRBuilder<> &builder, typed_value_vector &args) const {
  boost::function<codegen_value (vector<typed_value> &)> op = [this, module, &builder] (vector<typed_value> &args) -> codegen_value {
    if (args.size() != 4) return arg_count_mismatch(4, args.size());
    type_spec f = types->at("float");
    for (unsigned int i = 0; i < 4; i++) {
      if (f != args[i].get<1>()) {
	stringstream ss;
	ss << "Error in vec4 constructor argument " << i << ": Expected '" << f->name << "' found '" << args[i].get<1>()->name << "'.";
	return compile_error(ss.str());
      }
    }

    return make_llvm_float4(module, builder, *types,
			    args[0].get<0>(), args[1].get<0>(), args[2].get<0>(), args[3].get<0>());
  };
  errors::value_container_operation<typed_value_vector, codegen_value> constructor(op);
  return boost::apply_visitor(constructor, args);
}

codegen_value float4_type::op_add(Module *module, IRBuilder<> &builder,
				 codegen_value &lhs, codegen_value &rhs) const {
  typedef errors::argument_value_join<codegen_value, codegen_value>::result_value_type arg_val_type;
  boost::function<codegen_value (arg_val_type &)> op = [this, &builder, module] (arg_val_type &val) {
    return llvm_builtin_binop("rtl_add_v4v4", llvm_type(),
			      val.get<0>(), val.get<1>(), module, builder);
  };
  return errors::codegen_call_args(op, lhs, rhs);
}

//Float 3

float3_type::float3_type(type_table *types) :
  type(types, "vec3", "v3", true),
  type_value(StructType::create(getGlobalContext(),
				ArrayRef<Type*>(vector<Type*>(3, Type::getFloatTy(getGlobalContext()))),
				"vec3", true))
{
  
}

Type *float3_type::llvm_type() const {
  return type_value;
}

codegen_value float3_type::create(Module *module, IRBuilder<> &builder, typed_value_vector &args) const {
  boost::function<codegen_value (vector<typed_value> &)> op = [this, module, &builder] (vector<typed_value> &args) -> codegen_value {
    if (args.size() != 3) return arg_count_mismatch(3, args.size());
    type_spec f = types->at("float");
    for (unsigned int i = 0; i < 3; i++) {
      if (f != args[i].get<1>()) {
	stringstream ss;
	ss << "Error in vec4 constructor argument " << i << ": Expected '" << f->name << "' found '" << args[i].get<1>()->name << "'.";
	return compile_error(ss.str());
      }
    }

    return make_llvm_float3(module, builder, *types,
			    args[0].get<0>(), args[1].get<0>(), args[2].get<0>());
  };
  errors::value_container_operation<typed_value_vector, codegen_value> constructor(op);
  return boost::apply_visitor(constructor, args);
}

codegen_value float3_type::op_add(Module *module, IRBuilder<> &builder,
				 codegen_value &lhs, codegen_value &rhs) const {
  typedef errors::argument_value_join<codegen_value, codegen_value>::result_value_type arg_val_type;
  boost::function<codegen_value (arg_val_type &)> op = [this, &builder, module] (arg_val_type &val) {
    return llvm_builtin_binop("rtl_add_v3v3", llvm_type(),
			      val.get<0>(), val.get<1>(), module, builder);
  };
  return errors::codegen_call_args(op, lhs, rhs);
}

//Scene Pointer

Type *scene_ptr_type::llvm_type() const {
  return Type::getIntNPtrTy(getGlobalContext(), 8*sizeof(void*));
}
