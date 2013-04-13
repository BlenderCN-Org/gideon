#include "compiler/types/vector.hpp"
#include "compiler/operations.hpp"

using namespace raytrace;
using namespace llvm;
using namespace std;

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

//Float 2

float2_type::float2_type(type_table *types) :
  type(types, "vec2", "v2", true),
  type_value(StructType::create(getGlobalContext(),
				ArrayRef<Type*>(vector<Type*>(2, Type::getFloatTy(getGlobalContext()))),
				"vec2", true))
{
  
}

Type *float2_type::llvm_type() const {
  return type_value;
}

codegen_value float2_type::create(Module *module, IRBuilder<> &builder, typed_value_vector &args) const {
  boost::function<codegen_value (vector<typed_value> &)> op = [this, module, &builder] (vector<typed_value> &args) -> codegen_value {
    if (args.size() != 2) return arg_count_mismatch(2, args.size());
    type_spec f = types->at("float");
    for (unsigned int i = 0; i < 2; i++) {
      if (f != args[i].get<1>()) {
	stringstream ss;
	ss << "Error in vec4 constructor argument " << i << ": Expected '" << f->name << "' found '" << args[i].get<1>()->name << "'.";
	return compile_error(ss.str());
      }
    }

    return make_llvm_float2(module, builder, *types,
			    args[0].get<0>(), args[1].get<0>());
  };
  errors::value_container_operation<typed_value_vector, codegen_value> constructor(op);
  return boost::apply_visitor(constructor, args);
}

codegen_value float2_type::op_add(Module *module, IRBuilder<> &builder,
				 codegen_value &lhs, codegen_value &rhs) const {
  typedef errors::argument_value_join<codegen_value, codegen_value>::result_value_type arg_val_type;
  boost::function<codegen_value (arg_val_type &)> op = [this, &builder, module] (arg_val_type &val) {
    return llvm_builtin_binop("rtl_add_v2v2", llvm_type(),
			      val.get<0>(), val.get<1>(), module, builder);
  };
  return errors::codegen_call_args(op, lhs, rhs);
}
