/*

  Copyright 2013 Curtis Andrus

  This file is part of Gideon.

  Gideon is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  Gideon is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with Gideon.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "compiler/types/array.hpp"
#include "compiler/llvm_helper.hpp"

using namespace std;
using namespace raytrace;
using namespace llvm;

array_type::array_type(type_table *types,
		       const type_spec &base,
		       unsigned int N) :
  type(types, type_name(base, N), type_code(base, N)),
  base(base),
  N(N),
  arr_type(ArrayType::get(base->llvm_type(), N))
{
  
}

Type *array_type::llvm_type() const { return arr_type; }

string array_type::type_name(const type_spec &base, unsigned int N) {
  stringstream ss;
  ss << base->name << "[" << N << "]";
  return ss.str();
}

string array_type::type_code(const type_spec &base, unsigned int N) {
  stringstream ss;
  ss << base->type_id << "[" << N << "]";
  return ss.str();
}

typed_value_container array_type::initialize(llvm::Module *module, llvm::IRBuilder<> &builder) const {
  Value *array_val = UndefValue::get(arr_type);
  typed_value_container elem_init = base->initialize(module, builder);

  boost::function<typed_value_container (typed_value &)> load_init = [this, array_val, module, &builder] (typed_value &arg) -> typed_value_container {
    Value *init_val = arg.get<0>().extract_value();
    type_spec init_ty = arg.get<1>();
    Value *rt_val = array_val;

    if (init_val) {
      for (unsigned int i = 0; i < N; ++i) {
	rt_val = builder.CreateInsertValue(array_val, init_val, ArrayRef<unsigned int>(i));
      }
    }

    return typed_value(rt_val, types->get_array(base, N));
  };
  
  return errors::codegen_call(elem_init, load_init);
}

codegen_void array_type::destroy(Value *value, Module *module, IRBuilder<> &builder) {
  for (unsigned int i = 0; i < N; ++i) {
    Value *elem = builder.CreateConstGEP1_32(value, i);
    base->destroy(elem, module, builder);
  }

  return empty_type();
}

typed_value_container array_type::access_field(const string &field, Value *value,
					       Module *module, IRBuilder<> &builder) const {
  if (field == "length")
    return typed_value(ConstantInt::get(getGlobalContext(),
					APInt(8*sizeof(int), N, true)),
		       types->at("int"));
  
  stringstream ss;
  ss << "Array has no field named '" << field << "'";
  return errors::make_error<errors::error_message>(ss.str(), 0, 0);
}

typed_value_container array_type::access_element(Value *value, Value *elem_idx,
						 Module *module, IRBuilder<> &builder) const {
  Value *arr_ptr = CreateEntryBlockAlloca(builder, arr_type, "tmp_array");
  builder.CreateStore(value, arr_ptr);

  vector<Value*> idx({ConstantInt::get(getGlobalContext(), APInt(32, 0, true)), elem_idx});
  Value *elem_ptr = builder.CreateGEP(arr_ptr, idx, "array_elem_ptr");
  return typed_value(builder.CreateLoad(elem_ptr, "array_elem"), base);
}

typed_value_container array_type::access_element_ptr(Value *value_ptr, Value *elem_idx,
						     Module *module, IRBuilder<> &builder) const {
  vector<Value*> idx({ConstantInt::get(getGlobalContext(), APInt(32, 0, true)), elem_idx});
  Value *elem_ptr = builder.CreateGEP(value_ptr, idx, "array_elem_ptr");
  return typed_value(elem_ptr, base);
}

typed_value_container array_type::create(Module *module, IRBuilder<> &builder,
					 typed_value_vector &args, const type_conversion_table &conversions) const {
  boost::function<typed_value_container (vector<typed_value> &)> ctor = [this, module, &builder] (vector<typed_value> &args) -> typed_value_container {
    //check arg vector size
    if (args.size() != N) {
      stringstream ss;
      ss << "Array constructor expects " << N << " arguments, found " << args.size();
      return errors::make_error<errors::error_message>(ss.str(), 0, 0);
    }

    //create the array
    Value *array_val = UndefValue::get(arr_type);
    unsigned int idx = 0;

    //check that each argument has the correct type and build the array
    for (auto it = args.begin(); it != args.end(); ++it, ++idx) {
      type_spec arg_ty = it->get<1>();
      if (*arg_ty != *base) {
	stringstream ss;
	ss << "Invalid type '" << arg_ty->name << "' in " << base-> name << " array";
	return errors::make_error<errors::error_message>(ss.str(), 0, 0);
      }

      array_val = builder.CreateInsertValue(array_val, it->get<0>().extract_value(), ArrayRef<unsigned int>(idx));
    }
    
    return typed_value(array_val, types->get_array(base, N));
  };

  return errors::codegen_call<typed_value_vector, typed_value_container>(args, ctor);
}

/** Array Reference **/

array_reference_type::array_reference_type(type_table *types, const type_spec &base) :
  type(types, type_name(base), type_code(base)),
  base(base),
  ref_type(StructType::create(name,
			      Type::getInt32Ty(getGlobalContext()),
			      base->llvm_ptr_type(),
			      NULL))
{
  
}

string array_reference_type::type_name(const type_spec &base) {
  stringstream ss;
  ss << base->name << "[]";
  return ss.str();
}

string array_reference_type::type_code(const type_spec &base) {
  stringstream ss;
  ss << base->type_id << "[]";
  return ss.str();
}

Type *array_reference_type::llvm_type() const { return ref_type; }

typed_value_container array_reference_type::initialize(Module *module, IRBuilder<> &builder) const {
  return errors::make_error<errors::error_message>("No default initialization for array reference.", 0, 0);
}

typed_value_container array_reference_type::access_field(const string &field, Value *value,
							 Module *module, IRBuilder<> &builder) const {
  if (field == "length")
    return typed_value(builder.CreateExtractValue(value, ArrayRef<unsigned int>(0)),
		       types->at("int"));
  
  stringstream ss;
  ss << "Array has no field named '" << field << "'";
  return errors::make_error<errors::error_message>(ss.str(), 0, 0);
}

typed_value_container array_reference_type::access_element(Value *value, Value *elem_idx,
							   Module *module, IRBuilder<> &builder) const {
  Value *arr_ptr = builder.CreateExtractValue(value, ArrayRef<unsigned int>(1));
  return typed_value(builder.CreateLoad(builder.CreateGEP(arr_ptr, elem_idx)), base);
}

typed_value_container array_reference_type::access_element_ptr(Value *value_ptr, Value *elem_idx,
							       Module *module, IRBuilder<> &builder) const {
  Value *arr_ptr = builder.CreateLoad(builder.CreateStructGEP(value_ptr, 1));
  return typed_value(builder.CreateGEP(arr_ptr, elem_idx), base);
}


