#include "compiler/types.hpp"
#include "math/vector.hpp"
#include "geometry/ray.hpp"

#include "compiler/types/primitive.hpp"
#include "compiler/types/vector.hpp"
#include "compiler/types/special.hpp"
#include "compiler/types/array.hpp"

#include "compiler/type_conversion.hpp"

#include "compiler/llvm_helper.hpp"

#include "llvm/LLVMContext.h"
#include "llvm/ADT/ArrayRef.h"

#include <vector>
#include <stdexcept>

using namespace raytrace;
using namespace std;
using namespace llvm;

/** Type Table **/

type_spec type_table::get_array(const type_spec &base, unsigned int N) {
  string arr_ty_name = array_type::type_name(base, N);

  auto it = array_types.find(arr_ty_name);
  if (it != array_types.end()) return it->second;

  type_spec arr_type(new array_type(this, base, N));
  array_types.insert(make_pair(arr_ty_name, arr_type));
  return arr_type;
}

type_spec type_table::get_array_ref(const type_spec &base) {
  string arr_ty_name = array_reference_type::type_name(base);

  auto it = array_types.find(arr_ty_name);
  if (it != array_types.end()) return it->second;

  type_spec arr_type(new array_reference_type(this, base));
  array_types.insert(make_pair(arr_ty_name, arr_type));
  return arr_type;
}

size_t raytrace::hash_value(const type_spec &ts) {
  return boost::hash<type*>()(ts.get());
}

void raytrace::initialize_types(type_table &tt) {
  typedef shared_ptr<type> type_ptr;
  
  tt["void"] = type_ptr(new void_type(&tt));
  tt["bool"] = type_ptr(new bool_type(&tt));
  tt["int"] = type_ptr(new int_type(&tt));
  tt["float"] = type_ptr(new float_type(&tt));
  tt["string"] = type_ptr(new string_type(&tt));
  
  tt["vec2"] = type_ptr(new floatN_type(&tt, 2));
  tt["vec3"] = type_ptr(new floatN_type(&tt, 3));
  tt["vec4"] = type_ptr(new floatN_type(&tt, 4));
  
  tt["scene_ptr"] = type_ptr(new scene_ptr_type(&tt));
  tt["ray"] = type_ptr(new ray_type(&tt));
  tt["isect"] = type_ptr(new intersection_type(&tt));
  tt["light"] = type_ptr(new light_type(&tt));

  tt["dfunc"] = type_ptr(new dfunc_type(&tt));
  tt["shader_handle"] = type_ptr(new shader_handle_type(&tt));
  tt["context_ptr"] = type_ptr(new context_ptr_type(&tt));
  
  tt["module"] = type_ptr(new module_type(&tt));
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

typed_value_container type::create(Module *module, IRBuilder<> &builder, typed_value_vector &args,
				   const type_conversion_table &conversions) const {
  stringstream ss;
  ss << "Type '" << name << "' has no constructor.";
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
