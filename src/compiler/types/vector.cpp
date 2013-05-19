#include "compiler/types/vector.hpp"
#include "compiler/operations.hpp"

using namespace raytrace;
using namespace llvm;
using namespace std;

/* Vector Helper Functions */

typed_value_container make_llvm_vec_N(IRBuilder<> &builder, const string &vname, unsigned int N,
				      Type *vec_type, type_spec result_type, type_spec elem_type, typed_value_vector &args) {
  boost::function<typed_value_container (vector<typed_value> &)> op = [vec_type, result_type, elem_type, &vname, N, &builder] (vector<typed_value> &args) -> typed_value_container {
    //check argument count
    if (args.size() != N) {
      stringstream err_ss;
      err_ss << vname << " constructor expects " << N << "arguments, received " << args.size();
      return errors::make_error<errors::error_message>(err_ss.str(), 0, 0);
    }
    
    //check argument types
    for (unsigned int i = 0; i < N; ++i) {
      if (elem_type != args[i].get<1>()) {
	stringstream ss;
	ss << "Error in " << vname << " constructor argument " << i << ": Expected '" << elem_type->name << "' found '" << args[i].get<1>()->name << "'.";
	return errors::make_error<errors::error_message>(ss.str(), 0, 0);
      }
    }

    //build the vector
    string val_name = string("new_") + vname;
    Value *v = builder.CreateInsertValue(UndefValue::get(vec_type),
					 args[0].get<0>().extract_value(), ArrayRef<unsigned int>(0), val_name);
    for (unsigned int i = 1; i < N; ++i) {
      v = builder.CreateInsertValue(v, args[i].get<0>().extract_value(), ArrayRef<unsigned int>(i));
    }

    return typed_value(v, result_type);
  };

  errors::value_container_operation<typed_value_vector, typed_value_container> constructor(op);
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
  return errors::make_error<errors::error_message>(ss.str(), 0, 0);
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

typed_value_container floatN_type::access_field(const string &field, Value *value,
						Module *, IRBuilder<> &builder) const {
  unsigned int idx = 0;
  
  if (field.size() != 1) return invalid_field(field);
  if (!get_element_idx(field[0], idx)) return invalid_field(field);

  return typed_value(builder.CreateExtractValue(value, ArrayRef<unsigned int>(idx), "vec_elem"), types->at("float"));
}

typed_value_container floatN_type::access_field_ptr(const string &field, Value *value_ptr,
						    Module *module, IRBuilder<> &builder) const { 
  unsigned int idx = 0;
  
  if (field.size() != 1) return invalid_field(field);
  if (!get_element_idx(field[0], idx)) return invalid_field(field);
  
  return typed_value(builder.CreateStructGEP(value_ptr, idx, "vec_elem"), types->at("float"));
}

typed_value_container floatN_type::create(Module *, llvm::IRBuilder<> &builder, typed_value_vector &args) const {
  return make_llvm_vec_N(builder, type_name(N), N, type_value, types->at(type_name(N)), types->at("float"), args);
}
