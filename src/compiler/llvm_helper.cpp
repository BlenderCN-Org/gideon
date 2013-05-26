#include "compiler/llvm_helper.hpp"

using namespace raytrace;
using namespace llvm;
using namespace std;

Value *raytrace::CreateEntryBlockAlloca(IRBuilder<> &builder, Type *ty, const string &name) {
  Function *curr_func = builder.GetInsertBlock()->getParent();
  IRBuilder<> tmp(&curr_func->getEntryBlock(),
		  curr_func->getEntryBlock().begin());
  return tmp.CreateAlloca(ty, 0, name.c_str());
}

Value *raytrace::CreateEntryBlockArrayAlloca(IRBuilder<> &builder, 
					     Type *ty, Value *arr_size,
					     const string &name) {
  Function *curr_func = builder.GetInsertBlock()->getParent();
  IRBuilder<> tmp(&curr_func->getEntryBlock(),
		  curr_func->getEntryBlock().begin());
  return tmp.CreateAlloca(ty, arr_size, name.c_str());
}
