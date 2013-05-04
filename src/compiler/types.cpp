#include "compiler/types.hpp"
#include "math/vector.hpp"
#include "geometry/ray.hpp"

#include "compiler/types/primitive.hpp"
#include "compiler/types/vector.hpp"
#include "compiler/types/special.hpp"

#include "llvm/LLVMContext.h"
#include "llvm/ADT/ArrayRef.h"

#include <vector>
#include <stdexcept>

using namespace raytrace;
using namespace std;
using namespace llvm;

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
  tt["context_ptr"] = type_ptr(new context_ptr_type(&tt));
  
  tt["module"] = type_ptr(new module_type(&tt));
}

/** Type Base Class **/

typed_value_container type::create(Module *module, IRBuilder<> &builder, typed_value_vector &args) const {
  stringstream ss;
  ss << "Type '" << name << "' has no constructor.";
  return compile_error(ss.str());
}

typecheck_value type::field_type(const string &field) const {
  stringstream ss;
  ss << "Type '" << name << "' has no field named '" << field << "'.";
  return compile_error(ss.str());
}

typed_value_container type::access_field(const string &field, Value *value,
					 Module *, IRBuilder<> &) const {
  stringstream ss;
  ss << "Type '" << name << "' has no field named '" << field << "'.";
  return compile_error(ss.str());
}

typed_value_container type::access_field_ptr(const string &field, Value *value_ptr,
					     Module *, IRBuilder<> &) const {
  stringstream ss;
  ss << "Type '" << name << "' has no assignable field named '" << field << "'.";
  return compile_error(ss.str());
}

codegen_value type::op_add(Module *module, IRBuilder<> &builder,
			   codegen_value &lhs, codegen_value &rhs) const {
  stringstream ss;
  ss << "Type '" << name << "' does not support addition.";
  return compile_error(ss.str());
}

codegen_value type::op_sub(Module *module, IRBuilder<> &builder,
			   codegen_value &lhs, codegen_value &rhs) const {
  stringstream ss;
  ss << "Type '" << name << "' does not support subtraction.";
  return compile_error(ss.str());
}

codegen_value type::op_mul(Module *module, IRBuilder<> &builder,
			   codegen_value &lhs, codegen_value &rhs) const {
  stringstream ss;
  ss << "Type '" << name << "' does not support multiplication.";
  return compile_error(ss.str());
}

codegen_value type::op_div(Module *module, IRBuilder<> &builder,
			   codegen_value &lhs, codegen_value &rhs) const {
  stringstream ss;
  ss << "Type '" << name << "' does not support division.";
  return compile_error(ss.str());
}

codegen_value type::op_less(Module *module, IRBuilder<> &builder,
			   codegen_value &lhs, codegen_value &rhs) const {
  stringstream ss;
  ss << "Type '" << name << "' does not support less than comparison.";
  return compile_error(ss.str());
}

compile_error type::arg_count_mismatch(unsigned int expected, unsigned int found) const {
  stringstream ss;
  ss << "Error with type '" << name << "': Expected " << expected << " arguments, found " << found << ".";
  return compile_error(ss.str());
}
