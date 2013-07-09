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

#include "compiler/ast/control.hpp"

using namespace std;
using namespace raytrace;
using namespace llvm;

/* If-Else Condition */

raytrace::ast::conditional_statement::conditional_statement(parser_state *st, const expression_ptr &cond,
							    const statement_ptr &if_branch, const statement_ptr &else_branch,
							    unsigned int line_no, unsigned int column_no) :
  statement(st, line_no, column_no),
  cond(cond), if_branch(if_branch), else_branch(else_branch)
{

}

codegen_void raytrace::ast::conditional_statement::codegen(Module *module, IRBuilder<> &builder) {
  typed_value_container cond_val = cond->codegen(module, builder);
  cond_val = errors::codegen_call(cond_val, [this] (typed_value &cond) -> typed_value_container {
      if (*cond.get<1>() != *state->types["bool"]) return errors::make_error<errors::error_message>("Condition must be a boolean expression", line_no, column_no);
      return cond;
    });
  
  boost::function<codegen_void (typed_value &)> op = [this, module, &builder] (typed_value &cond) -> codegen_void {
    Value *cond_val = cond.get<0>().extract_value();
    Function *func = builder.GetInsertBlock()->getParent();
    
    BasicBlock *if_bb = BasicBlock::Create(getGlobalContext(), "if_bb", func);
    BasicBlock *else_bb = BasicBlock::Create(getGlobalContext(), "else_bb");
    BasicBlock *merge_bb = BasicBlock::Create(getGlobalContext(), "merge_bb");
    
    builder.CreateCondBr(cond_val, if_bb, else_bb);
    
    //generate code for if-block
    builder.SetInsertPoint(if_bb);
    
    codegen_void if_val = (if_branch ? if_branch->codegen(module, builder) : empty_type());
    if (!builder.GetInsertBlock()->getTerminator()) builder.CreateBr(merge_bb); //only go to the merge block if we don't return
    
    if_bb = builder.GetInsertBlock();
    
    //generate code for else-block
    func->getBasicBlockList().push_back(else_bb);
    builder.SetInsertPoint(else_bb);
    
    codegen_void else_val = (else_branch ? else_branch->codegen(module, builder) : empty_type());
    if (!builder.GetInsertBlock()->getTerminator()) builder.CreateBr(merge_bb);
    
    else_bb = builder.GetInsertBlock();
    
    //merge block
    func->getBasicBlockList().push_back(merge_bb);
    builder.SetInsertPoint(merge_bb);

    typedef errors::argument_value_join<codegen_void, codegen_void>::result_value_type arg_val_type;
    boost::function<codegen_void (arg_val_type &)> check_branches = [] (arg_val_type &) -> codegen_void { return empty_type(); };
    return errors::codegen_call_args(check_branches, if_val, else_val);
  };
  
  codegen_void result = errors::codegen_call<typed_value_container, codegen_void>(cond_val, op);
  if (!cond->bound()) ast::expression::destroy_unbound(cond_val, module, builder);
  state->control.set_scope_reaches_end(true);
  
  return result;
}

/* For Loop Statement */

raytrace::ast::for_loop_statement::for_loop_statement(parser_state *st,
						      const statement_ptr &init,
						      const expression_ptr &cond,
						      const expression_ptr &after,
						      const statement_ptr &body,
						      unsigned int line_no, unsigned int column_no) :
  statement(st, line_no, column_no), init(init), body(body), cond(cond), after(after)
{

}

codegen_void raytrace::ast::for_loop_statement::codegen(Module *module, IRBuilder<> &builder) {
  push_scope();
  
  Function *func = builder.GetInsertBlock()->getParent();
  BasicBlock *pre_bb = BasicBlock::Create(getGlobalContext(), "pre_bb", func);
  BasicBlock *loop_bb = BasicBlock::Create(getGlobalContext(), "loop_bb");
  BasicBlock *next_bb = BasicBlock::Create(getGlobalContext(), "next_bb");
  BasicBlock *post_bb = BasicBlock::Create(getGlobalContext(), "post_bb");
  
  state->control.push_loop(post_bb, next_bb);

  //initialize the loop
  codegen_void init_val = (init ? init->codegen(module, builder) : empty_type());

  //jump into the loop condition check
  builder.CreateBr(pre_bb);
  builder.SetInsertPoint(pre_bb);

  //evaluate and ensure that the loop condition is a bool
  typed_value_container cond_test = cond->codegen(module, builder);
  boost::function<typed_value_container (typed_value &)> bool_check = [this, &builder, loop_bb, post_bb] (typed_value &arg) -> typed_value_container {
    if (*arg.get<1>() != *state->types["bool"]) return errors::make_error<errors::error_message>("Condition must be a boolean expression", line_no, column_no);
    
    builder.CreateCondBr(arg.get<0>().extract_value(), loop_bb, post_bb);
    return arg;
  };

  cond_test = errors::codegen_call(cond_test, bool_check);

  func->getBasicBlockList().push_back(loop_bb);
  builder.SetInsertPoint(loop_bb);
    
  //execute the body
  codegen_void body_val = (body ? body->codegen(module, builder) : empty_type());
  if (!builder.GetInsertBlock()->getTerminator()) builder.CreateBr(next_bb); //return to the start of the loop
  
  func->getBasicBlockList().push_back(next_bb);
  builder.SetInsertPoint(next_bb);

  typed_value_container after_val = (after ? after->codegen(module, builder) : typed_value(nullptr, state->types["void"]));
  builder.CreateBr(pre_bb);
  
  //generate post-loop code
  func->getBasicBlockList().push_back(post_bb);
  builder.SetInsertPoint(post_bb);
  
  pop_scope(module, builder);
  state->control.pop_loop();

  //merge all the generated values to propagate any errors
  typedef raytrace::errors::argument_value_join<codegen_void, typed_value_container, codegen_void, typed_value_container>::result_value_type arg_val_type;
  boost::function<codegen_void (arg_val_type &)> final_check = [] (arg_val_type &) { return empty_type(); };
  codegen_void result = errors::codegen_call_args(final_check, init_val, cond_test, body_val, after_val);
  
  if (!cond->bound()) ast::expression::destroy_unbound(cond_test, module, builder);
  if (after && !after->bound()) ast::expression::destroy_unbound(after_val, module, builder);
  return result;
}

/** Break **/

raytrace::ast::break_statement::break_statement(parser_state *st,
						unsigned int line_no, unsigned int column_no) :
  statement(st, line_no, column_no)
{

}

codegen_void raytrace::ast::break_statement::codegen(Module *module, IRBuilder<> &builder) {
  if (builder.GetInsertBlock()->getTerminator()) return empty_type();
  if (!state->control.inside_loop()) return errors::make_error<errors::error_message>("Invalid use of 'break' outside any loop", line_no, column_no);
  state->control.set_scope_reaches_end(false);
  
  exit_loop_scopes(module, builder);
  BasicBlock *bb = state->control.post_loop();
  builder.CreateBr(bb);
  return empty_type();
}

/** Continue **/

raytrace::ast::continue_statement::continue_statement(parser_state *st,
						      unsigned int line_no, unsigned int column_no) :
  statement(st, line_no, column_no)
{

}

codegen_void raytrace::ast::continue_statement::codegen(Module *module, IRBuilder<> &builder) {
  if (builder.GetInsertBlock()->getTerminator()) return empty_type();
  if (!state->control.inside_loop()) return errors::make_error<errors::error_message>("Invalid use of 'continue' outside any loop", line_no, column_no);
  state->control.set_scope_reaches_end(false);

  exit_to_loop_scope(module, builder);
  BasicBlock *bb = state->control.next_iter();
  builder.CreateBr(bb);
  return empty_type();
}
