#ifndef GD_RL_LLVM_HELPER
#define GD_RL_LLVM_HELPER

#include "llvm/IRBuilder.h"
#include "llvm/Module.h"

namespace raytrace {

  //Inserts an alloca instruction at the beginning of the current function
  llvm::Value *CreateEntryBlockAlloca(llvm::IRBuilder<> &builder,
				      llvm::Type *ty, const std::string &name);
  
};

#endif
