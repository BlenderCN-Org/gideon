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

#include "compiler/types.hpp"
#include "math/vector.hpp"
#include "geometry/ray.hpp"

#include "compiler/types/primitive.hpp"
#include "compiler/types/vector.hpp"
#include "compiler/types/special.hpp"
#include "compiler/types/array.hpp"

#include "compiler/type_conversion.hpp"

#include "compiler/llvm_helper.hpp"

#include "llvm/IR/LLVMContext.h"
#include "llvm/ADT/ArrayRef.h"

#include <vector>
#include <stdexcept>

using namespace raytrace;
using namespace std;
using namespace llvm;

/** Type Table **/

type *type_table::add_nameless(std::unique_ptr<type> &&ptr) {
  size_t idx = nameless_types.size();
  nameless_types.push_back(move(ptr));
  return nameless_types[idx].get();
}

type *type_table::get_array(const type_spec &base, unsigned int N) {
  string arr_ty_name = array_type::type_name(base, N);

  auto it = array_types.find(arr_ty_name);
  if (it != array_types.end()) return it->second.get();

  array_types[arr_ty_name] = type_table::type_ptr(new array_type(this, base, N));
  return array_types[arr_ty_name].get();
}

type *type_table::get_array_ref(const type_spec &base) {
  string arr_ty_name = array_reference_type::type_name(base);

  auto it = array_types.find(arr_ty_name);
  if (it != array_types.end()) return it->second.get();

  array_types[arr_ty_name] = type_table::type_ptr(new array_reference_type(this, base));
  return array_types[arr_ty_name].get();
}

size_t raytrace::hash_value(const type_spec &ts) {
  return boost::hash<type*>()(ts);
}

void raytrace::initialize_types(type_table &tt) {
  tt.entry("void") = type_table::type_ptr(new void_type(&tt));
  tt.entry("bool") = type_table::type_ptr(new bool_type(&tt));
  tt.entry("int") = type_table::type_ptr(new int_type(&tt));
  tt.entry("float") = type_table::type_ptr(new float_type(&tt));
  tt.entry("string") = type_table::type_ptr(new string_type(&tt));
  
  tt.entry("vec2") = type_table::type_ptr(new floatN_type(&tt, 2));
  tt.entry("vec3") = type_table::type_ptr(new floatN_type(&tt, 3));
  tt.entry("vec4") = type_table::type_ptr(new floatN_type(&tt, 4));
  
  tt.entry("scene_ptr") = type_table::type_ptr(new scene_ptr_type(&tt));
  tt.entry("ray") = type_table::type_ptr(new ray_type(&tt));
  tt.entry("isect") = type_table::type_ptr(new intersection_type(&tt));
  tt.entry("light") = type_table::type_ptr(new light_type(&tt));

  tt.entry("dfunc") = type_table::type_ptr(new dfunc_type(&tt));
  tt.entry("shader_flag") = type_table::type_ptr(new shader_flag_type(&tt));
  tt.entry("shader_handle") = type_table::type_ptr(new shader_handle_type(&tt));
  tt.entry("context_ptr") = type_table::type_ptr(new context_ptr_type(&tt));
  
  tt.entry("module") = type_table::type_ptr(new module_type(&tt));
}

/** Type Base Class **/

Value *type::allocate(Module *, IRBuilder<> &builder) const {
  stringstream ss;
  ss << name << "_data";
  return CreateEntryBlockAlloca(builder, llvm_type(), ss.str());
}

Value *type::load(llvm::Value *ptr, Module *, IRBuilder<> &builder) const {
  return builder.CreateLoad(ptr);
}

void type::store(llvm::Value *value, llvm::Value *ptr, Module *, IRBuilder<> &builder) const {
  builder.CreateStore(value, ptr, false);
}

Type *type::llvm_ptr_type() const { return llvm_type()->getPointerTo(); }

code_value type::create(Module *module, IRBuilder<> &builder, typed_value_vector &args,
			const type_conversion_table &conversions) const {
  stringstream ss;
  ss << "Type '" << name << "' has no constructor.";
  return errors::make_error<errors::error_message>(ss.str(), 0, 0);
}

codegen_constant type::create_const(Module *module, IRBuilder<> &builder, codegen_const_vector &args,
				    const type_conversion_table &conversions) const {
  stringstream ss;
  ss << "Type '" << name << "' has no constant constructor.";
  return errors::make_error<errors::error_message>(ss.str(), 0, 0);
}

typecheck_value type::field_type(const string &field) const {
  stringstream ss;
  ss << "Type '" << name << "' has no field named '" << field << "'.";
  return errors::make_error<errors::error_message>(ss.str(), 0, 0);
}

typed_value_container type::access_field(const string &field, Value *value,
					 Module *, IRBuilder<> &) const {
  stringstream ss;
  ss << "Type '" << name << "' has no field named '" << field << "'.";
  return errors::make_error<errors::error_message>(ss.str(), 0, 0);
}

typed_value_container type::access_field_ptr(const string &field, Value *value_ptr,
					     Module *, IRBuilder<> &) const {
  stringstream ss;
  ss << "Type '" << name << "' has no assignable field named '" << field << "'.";
  return errors::make_error<errors::error_message>(ss.str(), 0, 0);
}

type_spec type::element_type() const {
  stringstream ss;
  ss << "Type '" << name << "' has no array elements.";
  throw runtime_error(ss.str());
}

typed_value_container type::access_element(Value *value, Value *elem_idx,
					   Module *module, IRBuilder<> &builder) const {
  stringstream ss;
  ss << "Type '" << name << "' has no array elements.";
  return errors::make_error<errors::error_message>(ss.str(), 0, 0);
}

typed_value_container type::access_element_ptr(Value *value_ptr, Value *elem_idx,
					       Module *module, IRBuilder<> &builder) const {
  stringstream ss;
  ss << "Type '" << name << "' has no array elements.";
  return errors::make_error<errors::error_message>(ss.str(), 0, 0);
}

compile_error type::arg_count_mismatch(unsigned int expected, unsigned int found) const {
  stringstream ss;
  ss << "Error with type '" << name << "': Expected " << expected << " arguments, found " << found << ".";
  return errors::make_error<errors::error_message>(ss.str(), 0, 0);
}
