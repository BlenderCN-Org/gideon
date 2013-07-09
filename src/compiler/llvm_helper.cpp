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
