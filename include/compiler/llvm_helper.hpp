#ifndef GD_RL_LLVM_HELPER
#define GD_RL_LLVM_HELPER

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"

namespace raytrace {

  //Inserts an alloca instruction at the beginning of the current function
  llvm::Value *CreateEntryBlockAlloca(llvm::IRBuilder<> &builder,
				      llvm::Type *ty, const std::string &name);

  llvm::Value *CreateEntryBlockArrayAlloca(llvm::IRBuilder<> &builder,
					   llvm::Type *ty, llvm::Value *arr_size,
					   const std::string &name);
  
  
};

#endif
