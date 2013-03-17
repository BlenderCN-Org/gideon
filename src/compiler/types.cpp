#include "compiler/types.hpp"
#include "math/vector.hpp"
#include "geometry/ray.hpp"

#include "llvm/LLVMContext.h"
#include "llvm/ADT/ArrayRef.h"

#include <vector>
#include <stdexcept>

using namespace raytrace;
using namespace std;
using namespace llvm;

/** Type Codes **/

namespace raytrace {
  template<> type_code get_type_code<bool>() { return BOOL; }
  template<> type_code get_type_code<int>() { return INT; }
  template<> type_code get_type_code<float>() { return FLOAT; }
  template<> type_code get_type_code<float4>() { return FLOAT4; }

  template<> type_code get_type_code<ray>() { return RAY; }
  template<> type_code get_type_code<void>() { return VOID; }
};

/** Type Traits **/

namespace raytrace {
  template<> type_traits get_type_traits<type_code::BOOL>() { return {"bool", "b", type_code::BOOL, true, true, sizeof(bool), NULL}; }
  template<> type_traits get_type_traits<type_code::INT>() { return {"int", "i", type_code::INT, true, true, sizeof(int), NULL}; }
  template<> type_traits get_type_traits<type_code::FLOAT>() { return {"float", "f", type_code::FLOAT, true, true, sizeof(float), NULL}; }

  template<> type_traits get_type_traits<type_code::FLOAT4>() { return {"vec4", "v4", type_code::FLOAT4, false, false, sizeof(float4), NULL}; }
  template<> type_traits get_type_traits<type_code::RAY>() { return {"ray", "r", type_code::RAY, false, true, sizeof(ray), NULL}; }

  template<> type_traits get_type_traits<type_code::VOID>() { return {"void", "v", type_code::VOID, true, true, 0, NULL}; }
  template<> type_traits get_type_traits<type_code::OTHER>() { return {"other", "0", type_code::OTHER, true, false, 0, NULL}; }
}

type_traits raytrace::get_type_traits(type_code t) {
  switch (t) {
  case BOOL: return get_type_traits<BOOL>();
  case INT: return get_type_traits<INT>();
  case FLOAT: return get_type_traits<FLOAT>();

  case FLOAT4: return get_type_traits<FLOAT4>();
  case RAY: return get_type_traits<RAY>();

  case VOID: return get_type_traits<VOID>();
  case OTHER:
  default:
    return get_type_traits<OTHER>();
  }
}

/** Destructors **/

bool raytrace::type_spec::base_equal(const type_spec &rhs) {
  return (t == rhs.t);
}

bool raytrace::type_spec::operator==(const type_spec &rhs) {
  return base_equal(rhs);
}

Type *raytrace::type_spec::llvm_type() const {
  return get_llvm_type(t);
}

void raytrace::type_spec::destroy(Value *addr, Module *module, IRBuilder<> &builder) {
  type_traits tt = get_type_traits(t);
  if (tt.destroy) tt.destroy(addr, module, builder);
}

string raytrace::type_spec::get_arg_code() const {
  type_traits tt = get_type_traits(t);
  return tt.code;
}

Type *raytrace::get_llvm_primitive_type(const type_code t) {
  switch (t) {
  case BOOL:
    return Type::getInt1Ty(getGlobalContext());
  case INT:
    return Type::getInt32Ty(getGlobalContext());
  case FLOAT:
    return Type::getFloatTy(getGlobalContext());
  case VOID:
    return Type::getVoidTy(getGlobalContext());
  case OTHER:
  default:
    throw runtime_error("Invalid type.");
  }
}

Type *raytrace::get_llvm_tuple_type(const type_code t, unsigned int n) {
  vector<Type*> types(n, get_llvm_primitive_type(t));
  return StructType::get(getGlobalContext(), ArrayRef<Type*>(types), true);
}

Type *raytrace::get_llvm_chunk_type(const type_code t) {
  type_traits tt = get_type_traits(t);
  size_t num_bytes = tt.size;
  return ArrayType::get(Type::getInt8Ty(getGlobalContext()), num_bytes);
}

Type *raytrace::get_llvm_type(const type_code t) {
  type_traits tt = get_type_traits(t);

  if (tt.is_primitive) return get_llvm_primitive_type(t);

  switch (t) {
  case FLOAT4:
    return get_llvm_tuple_type(FLOAT, 4);
  case RAY:
    return get_llvm_chunk_type(RAY);
  default:
    throw runtime_error("Invalid type.");
  }
  
}

/** Constructors **/

Value *raytrace::make_llvm_float4(Module *, IRBuilder<> &builder,
				  Value *x, Value *y, Value *z, Value *w) {
  llvm::Value *v = builder.CreateInsertValue(UndefValue::get(get_llvm_type(FLOAT4)),
					     x, ArrayRef<unsigned int>(0), "new_vec4");
  v = builder.CreateInsertValue(v, y, ArrayRef<unsigned int>(1));
  v = builder.CreateInsertValue(v, z, ArrayRef<unsigned int>(2));
  v = builder.CreateInsertValue(v, w, ArrayRef<unsigned int>(3));
  return v;
}
