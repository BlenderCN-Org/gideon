#include "compiler/types/special.hpp"
#include "compiler/llvm_helper.hpp"

#include "shading/distribution.hpp"
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

//Light Source

Type *light_type::llvm_type() const {
  return Type::getInt32PtrTy(getGlobalContext());
}

//Distribution Object

dfunc_type::dfunc_type(type_table *types) :
  type(types, "dfunc", "dist")
{
  
}

Type *dfunc_type::llvm_type() const {
  Type *byte_type = Type::getInt8Ty(getGlobalContext());
  return ArrayType::get(byte_type, sizeof(shade_tree::node_ptr));
}

codegen_value dfunc_type::initialize(Module *, IRBuilder<> &) const {
  return compile_error("No default initialization for distributions.");
}

Value *dfunc_type::copy(Value *value, Module *module, IRBuilder<> &builder) {
  Type *pointer_type = llvm_type()->getPointerTo();
  vector<Type*> arg_type({pointer_type, pointer_type});
  FunctionType *ty = FunctionType::get(Type::getVoidTy(getGlobalContext()), arg_type, false);
  Function *copy_f = cast<Function>(module->getOrInsertFunction("gd_builtin_copy_dfunc", ty));

  Value *val_ptr = CreateEntryBlockAlloca(builder, llvm_type(), "dfunc_src");
  builder.CreateStore(value, val_ptr, false);

  Value *copy = CreateEntryBlockAlloca(builder, llvm_type(), "dfunc_dst");
  builder.CreateCall2(copy_f, val_ptr, copy);
  return builder.CreateLoad(copy);
}

codegen_void dfunc_type::destroy(Value *value, Module *module, IRBuilder<> &builder) {
  vector<Type*> arg_type({llvm_type()->getPointerTo()});
  FunctionType *ty = FunctionType::get(Type::getVoidTy(getGlobalContext()), arg_type, false);
  Function *dtor = cast<Function>(module->getOrInsertFunction("gd_builtin_destroy_dfunc", ty));

  builder.CreateCall(dtor, value);
  return nullptr;
}

//Context Pointer

Type *context_ptr_type::llvm_type() const {
  return Type::getInt32PtrTy(getGlobalContext());
}
