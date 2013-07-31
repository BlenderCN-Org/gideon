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

#ifndef GD_RL_LLVM_HELPER
#define GD_RL_LLVM_HELPER

#include <boost/unordered_map.hpp>

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"

#include "llvm/ExecutionEngine/SectionMemoryManager.h"

namespace raytrace {

  //Inserts an alloca instruction at the beginning of the current function
  llvm::Value *CreateEntryBlockAlloca(llvm::IRBuilder<> &builder,
				      llvm::Type *ty, const std::string &name);

  llvm::Value *CreateEntryBlockArrayAlloca(llvm::IRBuilder<> &builder,
					   llvm::Type *ty, llvm::Value *arr_size,
					   const std::string &name);

  llvm::Function *GetExternalFunction(llvm::Module *module, const std::string &name, llvm::FunctionType *ft);
  
  //Memory manager that we can use to control where the "__gd_scene" variable is mapped to.
  //Symbols loaded in this map have priority over other symbols.
  class SceneDataMemoryManager : public llvm::SectionMemoryManager {
  public:

    boost::unordered_map<std::string, void*> explicit_map;

    virtual void *getPointerToNamedFunction(const std::string &Name,
					    bool AbortOnFailure = true);

  };
  
};

#endif
