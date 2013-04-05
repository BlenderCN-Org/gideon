#include "compiler/types/primitive.hpp"
#include "compiler/operations.hpp"

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

//Scene Pointer

Type *scene_ptr_type::llvm_type() const {
  return Type::getIntNPtrTy(getGlobalContext(), 8*sizeof(void*));
}
