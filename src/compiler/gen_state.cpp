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

#include "compiler/gen_state.hpp"
#include "compiler/llvm_helper.hpp"

using namespace raytrace;
using namespace llvm;
using namespace std;

control_state::function_state::function_state(Function *func,
					      const type_spec &t,
					      bool entry_point,
					      Module *module, IRBuilder<> &builder) :
  func(func), return_ty(t),
  rt_val(return_ty->is_void() ? nullptr : return_ty->allocate(module, builder)),
  target_sel(CreateEntryBlockAlloca(builder,
				    Type::getInt8Ty(getGlobalContext()), "target_sel")),
  exception(CreateEntryBlockAlloca(builder,
				   StructType::get(getGlobalContext(),
						   vector<Type*>{Type::getInt8PtrTy(getGlobalContext()), Type::getInt32Ty(getGlobalContext())}),
				   "eh_val")),
  entry_point(entry_point),
  entry_block(nullptr), cleanup_block(nullptr), return_block(nullptr), except_block(nullptr)
{
  
}

void control_state::push_function(const type_spec &t, bool entry_point, Function *f,
				  Module *module, IRBuilder<> &builder) {
  //setup the function's entry block
  BasicBlock *entry_block = BasicBlock::Create(getGlobalContext(), "entry", f);
  builder.SetInsertPoint(entry_block);
  
  function_stack.push_back(function_state(f, t, entry_point, module, builder));
  function_state &func_st = function_stack.back();
    
  //setup the return block
  BasicBlock *return_block = BasicBlock::Create(getGlobalContext(), "return", f);
  IRBuilder<> tmp(return_block, return_block->begin());
  if (t->is_void()) tmp.CreateRetVoid();
  else tmp.CreateRet(tmp.CreateLoad(func_st.rt_val));
  
  func_st.entry_block = entry_block;
  func_st.cleanup_block = nullptr;
  func_st.return_block = return_block;
  func_st.except_block = nullptr;
}



void control_state::pop_function() {
  function_stack.pop_back();
}

Function *control_state::get_current_function() {
  if (function_stack.size() == 0) return nullptr;
  return function_stack.back().func;
}

BasicBlock *control_state::get_function_body() {
  if (function_stack.size() == 0) return nullptr;
  return function_stack.back().entry_block;
}

BasicBlock *control_state::get_cleanup_block() {
  if (function_stack.size() == 0) return nullptr;
  return function_stack.back().cleanup_block;
}

BasicBlock *control_state::get_return_block() {
  if (function_stack.size() == 0) return nullptr;
  return function_stack.back().return_block;
}

Value *control_state::get_return_value_ptr() {
  if (function_stack.size() == 0) return nullptr;
  return function_stack.back().rt_val;
}

void control_state::set_jump_target(IRBuilder<> &builder, int code) {
  if (function_stack.size() == 0) return;
  builder.CreateStore(ConstantInt::get(getGlobalContext(), APInt(8, code, true)),
		      function_stack.back().target_sel);
}
    
void control_state::push_loop(BasicBlock *update) {
  loop_blocks loop{update, static_cast<unsigned int>(scope_state_stack.size())};
  loop_stack.push_back(loop);
}

bool control_state::at_loop_top() {
  if (loop_stack.size() == 0) return false;
  return (scope_state_stack.size() == loop_stack.back().top_scope);
}

void control_state::pop_loop() { loop_stack.pop_back(); }
bool control_state::inside_loop() { return loop_stack.size() > 0; }

BasicBlock *control_state::loop_update_block() { return loop_stack.back().update_block; }

void control_state::push_scope() {
  scope_state_stack.push_back(scope_state());
}

void control_state::pop_scope() {
  scope_state_stack.pop_back();
}

BasicBlock *control_state::get_exit_block() { 
  if (scope_state_stack.size() > 0) return scope_state_stack.back().exit_block;
  if (function_stack.size() > 0) return function_stack.back().return_block;
  return nullptr;
}

BasicBlock *control_state::get_error_block() { 
  if (scope_state_stack.size() > 0) return scope_state_stack.back().exit_block;
  if (function_stack.size() > 0) return function_stack.back().except_block;
  return nullptr;
}

BasicBlock *control_state::get_next_block() {
  if (scope_state_stack.size() > 0) return scope_state_stack.back().next_block;
  if (function_stack.size() > 0) return function_stack.back().return_block;
  return nullptr;
}

void control_state::push_context(Value *ctx, Type *ctx_type, const boost::function<void (Value*, Module*, IRBuilder<>&)> &loader) {
  class_context cc{ctx, ctx_type, loader};
  context_stack.push_back(cc);
}

void control_state::pop_context() { context_stack.pop_back(); }

bool control_state::has_context() { return (context_stack.size() > 0); }

void control_state::load_context(Module *module, IRBuilder<> &builder) {
  class_context &cc = context_stack.back();
  cc.loader(cc.ctx, module, builder);
}

Value *control_state::get_context() { return context_stack.back().ctx; }
void control_state::set_context(Value *ctx) { context_stack.back().ctx = ctx; }
Type *control_state::get_context_type() { return context_stack.back().ctx_type; }

MDNode *control_state::get_scope_metadata() {
  if (scope_state_stack.size() > 0) return scope_state_stack.back().metadata;
  if (function_stack.size() > 0) return function_stack.back().func_metadata;
  return nullptr;
}
