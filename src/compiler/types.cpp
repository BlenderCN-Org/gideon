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

void raytrace::initialize_types(type_table &tt) {
  typedef shared_ptr<type> type_ptr;
  
  tt["void"] = type_ptr(new void_type(&tt));
  tt["bool"] = type_ptr(new bool_type(&tt));
  tt["int"] = type_ptr(new int_type(&tt));
  tt["float"] = type_ptr(new float_type(&tt));
  tt["string"] = type_ptr(new string_type(&tt));

  tt["vec2"] = type_ptr(new float2_type(&tt));
  tt["vec3"] = type_ptr(new float3_type(&tt));
  tt["vec4"] = type_ptr(new float4_type(&tt));
  
  tt["scene_ptr"] = type_ptr(new scene_ptr_type(&tt));
  tt["ray"] = type_ptr(new ray_type(&tt));
  tt["isect"] = type_ptr(new intersection_type(&tt));
  tt["light"] = type_ptr(new light_type(&tt));
}

/** Type Base Class **/

codegen_value type::create(Module *module, IRBuilder<> &builder, typed_value_vector &args) const {
  stringstream ss;
  ss << "Type '" << name << "' has no constructor.";
  return compile_error(ss.str());
}

codegen_value type::op_add(Module *module, IRBuilder<> &builder,
			   codegen_value &lhs, codegen_value &rhs) const {
  stringstream ss;
  ss << "Type '" << name << "' does not support addition.";
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

/** Constructors **/

Value *raytrace::make_llvm_float2(Module *, IRBuilder<> &builder,
				  type_table &types,
				  Value *x, Value *y) {
  Value *v = builder.CreateInsertValue(UndefValue::get(types["vec2"]->llvm_type()),
				       x, ArrayRef<unsigned int>(0), "new_vec2");
  v = builder.CreateInsertValue(v, y, ArrayRef<unsigned int>(1));
  return v;
}

Value *raytrace::make_llvm_float3(Module *, IRBuilder<> &builder,
				  type_table &types,
				  Value *x, Value *y, Value *z) {
  llvm::Value *v = builder.CreateInsertValue(UndefValue::get(types["vec3"]->llvm_type()),
					     x, ArrayRef<unsigned int>(0), "new_vec3");
  v = builder.CreateInsertValue(v, y, ArrayRef<unsigned int>(1));
  v = builder.CreateInsertValue(v, z, ArrayRef<unsigned int>(2));
  return v;
}

Value *raytrace::make_llvm_float4(Module *, IRBuilder<> &builder,
				  type_table &types,
				  Value *x, Value *y, Value *z, Value *w) {
  llvm::Value *v = builder.CreateInsertValue(UndefValue::get(types["vec4"]->llvm_type()),
					     x, ArrayRef<unsigned int>(0), "new_vec4");
  v = builder.CreateInsertValue(v, y, ArrayRef<unsigned int>(1));
  v = builder.CreateInsertValue(v, z, ArrayRef<unsigned int>(2));
  v = builder.CreateInsertValue(v, w, ArrayRef<unsigned int>(3));
  return v;
}
