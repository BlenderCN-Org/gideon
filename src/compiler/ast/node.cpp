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

#include "compiler/ast/node.hpp"
#include "llvm/Support/Dwarf.h"

using namespace std;
using namespace raytrace;
using namespace llvm;

void ast::ast_node::push_symtab_scope(const string &name) {
  variables().scope_push(name);
  functions().scope_push(name);
}

void ast::ast_node::pop_symtab_scope(Module *module, IRBuilder<> &builder) {
  functions().scope_pop(module, builder, false);
  variables().scope_pop(module, builder, false);
}

void ast::ast_node::push_scope(Module *module, IRBuilder<> &builder) {
  //create cleanup block for this point in time
  BasicBlock *cleanup = generate_cleanup(module);
  MDNode *parent_md = state->control.get_scope_metadata();
  
  //push the new scope state
  state->control.push_scope();
  control_state::scope_state &scope = state->control.get_scope_state();

  variables().scope_push("");
  functions().scope_push("");

  //create a new body block for this scope
  BasicBlock *body = BasicBlock::Create(getGlobalContext(), "scope",
					state->control.get_function_state().func,
					cleanup);

  //this block is where the program will go once it leaves the scope normally
  BasicBlock *post = BasicBlock::Create(getGlobalContext(), "reentry",
					state->control.get_function_state().func,
					cleanup);
  
  //current block jumps into the new body
  builder.CreateBr(body);
  builder.SetInsertPoint(body);

  scope.body_block = body;
  scope.exit_block = cleanup;
  scope.next_block = post;
}

void ast::ast_node::pop_scope(Module *module, IRBuilder<> &builder) {
  //create an exit if this block doesn't already have a terminator
  control_state::scope_state &scope = state->control.get_scope_state();
  BasicBlock *next_block = state->control.get_next_block();

  if (!builder.GetInsertBlock()->getTerminator()) {
    
    //generate cleanup block for this scope
    BasicBlock *cleanup = generate_cleanup(module);
    
    //set target to 'next'
    state->control.set_jump_target(builder, 0);
    
    //branch into cleanup block
    builder.CreateBr(cleanup);
  }
  
  state->control.pop_scope();
  
  builder.SetInsertPoint(next_block);
  functions().scope_pop(module, builder, false);
  variables().scope_pop(module, builder, false);
  
}

void ast::ast_node::push_function(const type_spec &t, bool entry_point, Function *f,
				  Module *module, IRBuilder<> &builder) {
  //order is important here, we need to know which scope we can exit to
  state->control.push_function(t, entry_point, f, module, builder);
  control_state::function_state &func_st = state->control.get_function_state();
  
  variables().scope_push("");
  functions().scope_push("");
}
  
void ast::ast_node::pop_function(Module *module, IRBuilder<> &builder) {
  functions().scope_pop(module, builder, false);
  variables().scope_pop(module, builder, false);

  state->control.pop_function();
}

void ast::ast_node::push_module(const string &name, bool exported) {
  state->modules.scope_push(name);
  if (exported) state->exports.push_module(name);
}
 
codegen_void ast::ast_node::pop_module(const string &name, bool exported,
				       Module *module, IRBuilder<> &builder) {
  void_vector result;

  //save this module
  auto scope_it = state->modules.scope_begin();
  auto parent_scope = scope_it + 1;
  if (parent_scope != state->modules.scope_end()) {
    codegen_void saved = empty_type();
    module_ptr &module = scope_it->get_module();
    auto mod_it = parent_scope->get_module()->modules.find(name);
    if (mod_it != parent_scope->get_module()->modules.end()) {
      stringstream err_ss;
      err_ss << "Redeclaration of module '" << name << "'";
      saved = errors::make_error<errors::error_message>(err_ss.str(), line_no, column_no);
    }
    else parent_scope->get_module()->modules[name] = module;

    result = errors::codegen_vector_push_back(result, saved);
  }

  if (exported) state->exports.pop_module();
  state->modules.scope_pop(module, builder, false);

  return errors::codegen_call<void_vector, codegen_void>(result, [] (vector<empty_type> &arg) -> codegen_void {
      return empty_type();
    });
}

void ast::ast_node::push_distribution_context(const string &name, Type *param_ptr_type, const control_state::context_loader_type &loader) {
  state->control.push_context(NULL, param_ptr_type, loader);

  variables().scope_push(name);
  functions().scope_push(name);
}

void ast::ast_node::pop_distribution_context(Module *module, IRBuilder<> &builder) {
  functions().scope_pop(module, builder, false);
  variables().scope_pop(module, builder, false);

  state->control.pop_context();
}

variable_symbol_table &ast::ast_node::variables() {
  return state->variables;
}

variable_scope &ast::ast_node::global_variables() {
  return state->modules.scope().get_module()->variables;
}

function_scope &ast::ast_node::function_table() {
  if (state->functions.depth() > 0) return state->functions.scope();
  assert(state->modules.depth() > 0); //this shouldn't be called if there is nowhere to save a function
  return state->modules.scope().get_module()->functions;
}

string ast::ast_node::function_scope_name() {
  assert(state->modules.depth() > 0);

  stringstream ss;
  ss << state->modules.scope_name();
  if (state->functions.depth() > 0) ss << "_" << state->functions.scope_name();
  
  return ss.str();
}

function_symbol_table &ast::ast_node::functions() {
  return state->functions;
}

const function_symbol_table &ast::ast_node::functions() const {
  return state->functions;
}

typecheck_value ast::ast_node::variable_type_lookup(const string &name) {
  //lookup the name in the local variable table
  try {
    variable_symbol_table::entry_type &var = variables().get(name);
    return var.type;
  }
  catch (compile_error &e) { } //ignore and move on 

  //check globals and module names
  for (auto scope_it = state->modules.scope_begin(); scope_it != state->modules.scope_end(); ++scope_it) {
    module_ptr &module = scope_it->get_module();
    
    //look for a global variable name
    auto var_it = module->variables.find(name);
    if (var_it != module->variables.end()) {
      variable_symbol_table::entry_type &entry = var_it->second;
      return entry.type;
    }

    //check for a module name
    auto mod_it = module->modules.find(name);
    if (mod_it != module->modules.end()) {
      return state->types["module"];
    }
  }

  stringstream err_ss;
  err_ss << "No such variable or module named '" << name << "'";

  return errors::make_error<errors::error_message>(err_ss.str(), line_no, column_no);  
}

typed_value_container ast::ast_node::variable_lookup(const string &name) {
  //lookup the name in the local variable table
  try {
    variable_symbol_table::entry_type &var = variables().get(name);
    return typed_value(var.value, var.type);
  }
  catch (compile_error &e) { } //ignore and move on 

  //check globals and module names
  for (auto scope_it = state->modules.scope_begin(); scope_it != state->modules.scope_end(); ++scope_it) {
    module_ptr &module = scope_it->get_module();
    
    //look for a global variable name
    auto var_it = module->variables.find(name);
    if (var_it != module->variables.end()) {
      variable_symbol_table::entry_type &entry = var_it->second;
      return typed_value(entry.value, entry.type);
    }

    //check for a module name
    auto mod_it = module->modules.find(name);
    if (mod_it != module->modules.end()) {
      return typed_value(mod_it->second, state->types["module"]);
    }
  }

  stringstream err_ss;
  err_ss << "No such variable or module named '" << name << "'";
  return errors::make_error<errors::error_message>(err_ss.str(), line_no, column_no);
}

ast::ast_node::entry_or_error ast::ast_node::function_lookup(const function_key &fkey) {
  try {
    //search each scope in this module (starting at the closest, working backwards)
    for (auto fscope = functions().scope_begin(); fscope != functions().scope_end(); ++fscope) {
      auto f_it = fscope->find_best(fkey, state->type_conversions);
      if (f_it != fscope->end()) return &(*f_it);
    }
    
    //look up the module stack
    for (auto mod_scope = state->modules.scope_begin(); mod_scope != state->modules.scope_end(); ++mod_scope) {
      function_scope &fscope = mod_scope->get_module()->functions;
      auto f_it = fscope.find_best(fkey, state->type_conversions);
      if (f_it != fscope.end()) return &(*f_it);
    }
  }
  catch (runtime_error &e) {
    //ambiguous function call case
    return errors::make_error<errors::error_message>(e.what(), line_no, column_no);
  }

  stringstream err_ss;
  err_ss << "Undeclared function '" << fkey.name << "'";
  return errors::make_error<errors::error_message>(err_ss.str(), line_no, column_no);
}

code_value ast::ast_node::typecast(typed_value_container &src, const type_spec &dst_type,
				   bool make_copy, bool destroy_on_convert,
				   Module *module, IRBuilder<> &builder) {
  boost::function<code_value (typed_value &)> cast_op = [this, &dst_type,
							 make_copy, destroy_on_convert,
							 module, &builder] (typed_value &args) -> code_value {
    type_spec src_type = args.get<1>();
    
    Value *src = args.get<0>().extract_value();
    if (make_copy) src = src_type->copy(src, module, builder);
    
    code_value result = state->type_conversions.convert(src_type, src, dst_type, module, builder);
    if ((*src_type != *dst_type) && destroy_on_convert) src_type->destroy(src, module, builder);

    return result;
  };

  return errors::codegen_call<typed_value_container, code_value>(src, cast_op);
}

code_value ast::ast_node::typecast(Value *src, 
				   const type_spec &src_type, const type_spec &dst_type,
				   bool make_copy, bool destroy_on_convert,
				   Module *module, IRBuilder<> &builder) {
  if (make_copy) src = src_type->copy(src, module, builder);    
  code_value result = state->type_conversions.convert(src_type, src, dst_type, module, builder);
  
  if ((*src_type != *dst_type) && destroy_on_convert) src_type->destroy(src, module, builder);
  return result;
}

typecheck_value ast::ast_node::typename_lookup(const string &name) {
  //lookup the name in the local variable table
  try {
    return state->user_types.get(name);
  }
  catch (compile_error &e) { } //ignore and move on 

  //check defined in the modules
  for (auto scope_it = state->modules.scope_begin(); scope_it != state->modules.scope_end(); ++scope_it) {
    module_ptr &module = scope_it->get_module();

    auto type_it = module->types.find(name);
    if (type_it != module->types.end()) {
      return type_it->second;
    }
  }

  //check built-in types
  if (state->types.has_type(name)) return state->types[name];

  stringstream err_ss;
  err_ss << "No such type named '" << name << "'";
  return errors::make_error<errors::error_message>(err_ss.str(), line_no, column_no);
}

BasicBlock *ast::ast_node::generate_cleanup(Module *module) {
  control_state::function_state &func_st = state->control.get_function_state();

  Function *f = func_st.func;
  BasicBlock *next_block = state->control.get_next_block();
  BasicBlock *cleanup = BasicBlock::Create(getGlobalContext(), "cleanup", f, next_block);

  IRBuilder<> builder(cleanup, cleanup->begin());
  variables().scope_destroy(module, builder);
  functions().scope_destroy(module, builder);
  
  BasicBlock *exit = state->control.get_exit_block();
  BasicBlock *exception = state->control.get_error_block();
  
  int case_count = 3;
  if (state->control.inside_loop()) case_count += 2;

  SwitchInst* branch = builder.CreateSwitch(builder.CreateLoad(func_st.target_sel), exit, case_count);
  branch->addCase(ConstantInt::get(getGlobalContext(), APInt(8, 0, true)), next_block);
  branch->addCase(ConstantInt::get(getGlobalContext(), APInt(8, 1, true)), exit);

  if (state->control.inside_loop()) {
    BasicBlock *continue_block = state->control.at_loop_top() ? state->control.loop_update_block() : exit;
    branch->addCase(ConstantInt::get(getGlobalContext(), APInt(8, 2, true)), continue_block);

    BasicBlock *break_block = state->control.at_loop_top() ? next_block : exit;
    branch->addCase(ConstantInt::get(getGlobalContext(), APInt(8, 3, true)), break_block);
  }
  
  branch->addCase(ConstantInt::get(getGlobalContext(), APInt(8, 4, true)), exception);

  return cleanup;
}

void ast::ast_node::generate_return_branch(Module *module, IRBuilder<> &builder) {
  BasicBlock *cleanup = generate_cleanup(module);
  state->control.set_jump_target(builder, 1);
  builder.CreateBr(cleanup);
}

void ast::ast_node::generate_exception_propagate(Module *module) {
  control_state::function_state &func_st = state->control.get_function_state();

  BasicBlock *except_block = BasicBlock::Create(getGlobalContext(), "error", func_st.func);
  IRBuilder<> eh_tmp(except_block, except_block->begin());
  eh_tmp.CreateResume(eh_tmp.CreateLoad(func_st.exception));
  func_st.except_block = except_block;
}

void ast::ast_node::generate_exception_catch(Module *module) {
  control_state::function_state &func_st = state->control.get_function_state();
  BasicBlock *except_block = BasicBlock::Create(getGlobalContext(), "error", func_st.func);
  IRBuilder<> eh_tmp(except_block, except_block->begin());

  Type *int8_ptr = Type::getInt8PtrTy(getGlobalContext());
  FunctionType *begin_ty = FunctionType::get(int8_ptr, vector<Type*>{int8_ptr}, false);
  Function *begin_catch = cast<Function>(module->getOrInsertFunction("__cxa_begin_catch", begin_ty));

  Value *eh_ptr = eh_tmp.CreateLoad(eh_tmp.CreateStructGEP(func_st.exception, 0));

  FunctionType *end_ty = FunctionType::get(Type::getVoidTy(getGlobalContext()),
					   vector<Type*>(), false);
  Function *end_catch = cast<Function>(module->getOrInsertFunction("__cxa_end_catch", end_ty));

  Value *error_str_ptr = eh_tmp.CreateCall(begin_catch, eh_ptr);

  FunctionType *handle_ty = FunctionType::get(Type::getVoidTy(getGlobalContext()), vector<Type*>{int8_ptr}, false);
  Function *handle = cast<Function>(module->getOrInsertFunction("gd_builtin_handle_error", handle_ty));
  eh_tmp.CreateCall(handle, error_str_ptr);
  
  eh_tmp.CreateCall(end_catch);

  if (func_st.return_ty->is_void()) eh_tmp.CreateRetVoid();
  else eh_tmp.CreateRet(eh_tmp.CreateLoad(func_st.rt_val));

  func_st.except_block = except_block;
}

BasicBlock *ast::ast_node::generate_landing_pad(Module *module) {
  BasicBlock *cleanup = generate_cleanup(module);
  
  control_state::function_state &func_st = state->control.get_function_state();
  bool handle_error = func_st.entry_point;

  BasicBlock *lp_block = BasicBlock::Create(getGlobalContext(), "landing", func_st.func);
  IRBuilder<> lp_tmp(lp_block, lp_block->begin());
  
  FunctionType *personality_fty = FunctionType::get(Type::getInt32Ty(getGlobalContext()),
						    true);
  Function *personality_fn = cast<Function>(module->getOrInsertFunction("__gxx_personality_v0", personality_fty));

  vector<Type*> eh_types{Type::getInt8PtrTy(getGlobalContext()), Type::getInt32Ty(getGlobalContext())};
  StructType *eh_struct = StructType::get(getGlobalContext(), eh_types);

  unsigned int num_clauses = (handle_error ? 1 : 0);

  LandingPadInst *ex = lp_tmp.CreateLandingPad(eh_struct,
					       personality_fn, num_clauses);
  ex->setCleanup(true);
  if (handle_error) ex->addClause(ConstantPointerNull::get(Type::getInt8PtrTy(getGlobalContext())));
  
  state->types["string"]->store(cast<Value>(ex), func_st.exception, module, lp_tmp);
  state->control.set_jump_target(lp_tmp, 4);
  lp_tmp.CreateBr(cleanup);

  return lp_block;
}
