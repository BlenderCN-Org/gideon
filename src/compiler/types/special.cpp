#include "compiler/types/special.hpp"
#include "geometry/ray.hpp"

using namespace raytrace;
using namespace llvm;
using namespace std;

//Ray

Type *ray_type::llvm_type() const {
  Type *byte_type = Type::getInt8Ty(getGlobalContext());
  return ArrayType::get(byte_type, sizeof(ray));
}

//Intersection

Type *intersection_type::llvm_type() const {
  Type *byte_type = Type::getInt8Ty(getGlobalContext());
  return ArrayType::get(byte_type, sizeof(intersection));
}
