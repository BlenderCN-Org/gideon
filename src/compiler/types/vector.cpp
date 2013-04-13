#include "compiler/types/vector.hpp"
#include "compiler/operations.hpp"

using namespace raytrace;
using namespace llvm;
using namespace std;

/* Vector Helper Functions */

codegen_value make_llvm_vec_N(IRBuilder<> &builder, const string &vname, unsigned int N,
			      Type *vec_type, type_spec elem_type, typed_value_vector &args) {
  boost::function<codegen_value (vector<typed_value> &)> op = [vec_type, elem_type, &vname, N, &builder] (vector<typed_value> &args) -> codegen_value {
    //check argument count
    if (args.size() != N) {
      stringstream err_ss;
      err_ss << vname << " constructor expects " << N << "arguments, received " << args.size();
      return compile_error(err_ss.str());
    }
    
    //check argument types
    for (unsigned int i = 0; i < N; ++i) {
      if (elem_type != args[i].get<1>()) {
	stringstream ss;
	ss << "Error in " << vname << " constructor argument " << i << ": Expected '" << elem_type->name << "' found '" << args[i].get<1>()->name << "'.";
	return compile_error(ss.str());
      }
    }

    //build the vector
    string val_name = string("new_") + vname;
    Value *v = builder.CreateInsertValue(UndefValue::get(vec_type),
					 args[0].get<0>(), ArrayRef<unsigned int>(0), val_name);
    for (unsigned int i = 1; i < N; ++i) {
      v = builder.CreateInsertValue(v, args[i].get<0>(), ArrayRef<unsigned int>(i));
    }

    return v;
  };

  errors::value_container_operation<typed_value_vector, codegen_value> constructor(op);
  return boost::apply_visitor(constructor, args);
}

/* floatN_type Implementation */

string floatN_type::type_name(unsigned int N) {
  stringstream ss;
  ss << "vec" << N;
  return ss.str();
}

string floatN_type::type_code(unsigned int N) {
  stringstream ss;
  ss << "v" << N;
  return ss.str();
}

floatN_type::floatN_type(type_table *types, unsigned int N) :
  type(types, type_name(N), type_code(N), true),
  N(N),
  type_value(StructType::create(getGlobalContext(),
				ArrayRef<Type*>(vector<Type*>(N, Type::getFloatTy(getGlobalContext()))),
				type_name(N), true))
{

}

Type *floatN_type::llvm_type() const { return type_value; }

bool floatN_type::get_element_idx(char c, /* out */ unsigned int &idx) const {
  switch (c) {
  case 'x':
    idx = 0;
    break;
  case 'y':
    idx = 1;
    break;
  case 'z':
    idx = 2;
    break;
  case 'w':
    idx = 3;
    break;
  default:
    return false;
  };

  return (idx < N);
}

compile_error floatN_type::invalid_field(const string &field) const {
  stringstream ss;
  ss << type_name(N) << " has no field named '" << field << "'";
  return compile_error(ss.str());
}

typecheck_value floatN_type::field_type(const string &field) const {
  //check each character to make sure it's a valid element
  unsigned int idx;
  for (string::size_type i = 0; i < field.size(); ++i) {
    if (!get_element_idx(field[i], idx)) return invalid_field(field);
  }

  //for the moment, no swizzling
  if (field.size() != 1) return invalid_field(field);
  return types->at("float");
}

codegen_value floatN_type::access_field(const string &field, Value *value,
					Module *, IRBuilder<> &builder) const {
  unsigned int idx = 0;
  
  if (field.size() != 1) return invalid_field(field);
  if (!get_element_idx(field[0], idx)) return invalid_field(field);

  return builder.CreateExtractValue(value, ArrayRef<unsigned int>(idx), "vec_elem");
}

codegen_value floatN_type::access_field_ptr(const string &field, Value *value_ptr,
					    Module *module, IRBuilder<> &builder) const { 
  unsigned int idx = 0;
  
  if (field.size() != 1) return invalid_field(field);
  if (!get_element_idx(field[0], idx)) return invalid_field(field);
  
  return builder.CreateStructGEP(value_ptr, idx, "vec_elem");
}

codegen_value floatN_type::create(Module *, llvm::IRBuilder<> &builder, typed_value_vector &args) const {
  return make_llvm_vec_N(builder, type_name(N), N, type_value, types->at("float"), args);
}

codegen_value floatN_type::op_add(Module *module, IRBuilder<> &builder,
				  codegen_value &lhs, codegen_value &rhs) const {
  stringstream op_ss;
  op_ss << "gd_builtin_add_v" << N << "_v" << N;
  string op_func = op_ss.str();
  
  typedef errors::argument_value_join<codegen_value, codegen_value>::result_value_type arg_val_type;
  boost::function<codegen_value (arg_val_type &)> op = [this, &op_func, &builder, module] (arg_val_type &val) {
    return llvm_builtin_binop(op_func, llvm_type(),
			      val.get<0>(), val.get<1>(), module, builder);
  };
  return errors::codegen_call_args(op, lhs, rhs);
}

codegen_value floatN_type::op_sub(Module *module, IRBuilder<> &builder,
				  codegen_value &lhs, codegen_value &rhs) const {
  stringstream op_ss;
  op_ss << "gd_builtin_sub_v" << N << "_v" << N;
  string op_func = op_ss.str();
  
  typedef errors::argument_value_join<codegen_value, codegen_value>::result_value_type arg_val_type;
  boost::function<codegen_value (arg_val_type &)> op = [this, &op_func, &builder, module] (arg_val_type &val) {
    return llvm_builtin_binop(op_func, llvm_type(),
			      val.get<0>(), val.get<1>(), module, builder);
  };
  return errors::codegen_call_args(op, lhs, rhs);
}
