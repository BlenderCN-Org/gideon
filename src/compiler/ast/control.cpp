#include "compiler/ast/control.hpp"

using namespace std;
using namespace raytrace;
using namespace llvm;

/* If-Else Condition */

raytrace::ast::conditional_statement::conditional_statement(const expression_ptr &cond,
							    const statement_ptr &if_branch, const statement_ptr &else_branch) :
  statement(),
  cond(cond), if_branch(if_branch), else_branch(else_branch)
{

}

Value *raytrace::ast::conditional_statement::codegen(Module *module, IRBuilder<> &builder) {
  type_spec t = cond->typecheck();
  if (t.t != BOOL) throw runtime_error("Condition must be a boolean expression");

  Value *cond_val = cond->codegen(module, builder);
  Function *func = builder.GetInsertBlock()->getParent();

  BasicBlock *if_bb = BasicBlock::Create(getGlobalContext(), "if_bb", func);
  BasicBlock *else_bb = BasicBlock::Create(getGlobalContext(), "else_bb");
  BasicBlock *merge_bb = BasicBlock::Create(getGlobalContext(), "merge_bb");

  builder.CreateCondBr(cond_val, if_bb, else_bb);

  //generate code for if-block
  builder.SetInsertPoint(if_bb);

  if (if_branch) if_branch->codegen(module, builder);
  if (!builder.GetInsertBlock()->getTerminator()) builder.CreateBr(merge_bb); //only go to the merge block if we don't return

  if_bb = builder.GetInsertBlock();
  
  //generate code for else-block
  func->getBasicBlockList().push_back(else_bb);
  builder.SetInsertPoint(else_bb);

  if (else_branch) else_branch->codegen(module, builder);
  if (!builder.GetInsertBlock()->getTerminator()) builder.CreateBr(merge_bb);
  
  else_bb = builder.GetInsertBlock();

  //merge block
  func->getBasicBlockList().push_back(merge_bb);
  builder.SetInsertPoint(merge_bb);

  return cond_val;
}

/* For Loop Statement */

raytrace::ast::for_loop_statement::for_loop_statement(const statement_ptr &init,
						      const expression_ptr &cond,
						      const expression_ptr &after,
						      const statement_ptr &body,
						      var_symbol_table *variables, func_symbol_table *functions,
						      control_state *control) :
  init(init), body(body), cond(cond), after(after),
  variables(variables), functions(functions), control(control)
{

}

Value *raytrace::ast::for_loop_statement::codegen(Module *module, IRBuilder<> &builder) {
  type_spec t = cond->typecheck();
  if (t.t = type_code::BOOL) throw runtime_error("Condition must be a boolean expression");

  variables->scope_push();
  functions->scope_push();

  Function *func = builder.GetInsertBlock()->getParent();
  BasicBlock *pre_bb = BasicBlock::Create(getGlobalContext(), "pre_bb", func);
  BasicBlock *loop_bb = BasicBlock::Create(getGlobalContext(), "loop_bb");
  BasicBlock *next_bb = BasicBlock::Create(getGlobalContext(), "next_bb");
  BasicBlock *post_bb = BasicBlock::Create(getGlobalContext(), "post_bb");

  control->push_loop(post_bb, next_bb);

  //initialize the loop
  if (init) init->codegen(module, builder);

  //jump into the loop condition check
  builder.CreateBr(pre_bb);
  builder.SetInsertPoint(pre_bb);

  Value *cond_test = cond->codegen(module, builder);
  builder.CreateCondBr(cond_test, loop_bb, post_bb);
  
  func->getBasicBlockList().push_back(loop_bb);
  builder.SetInsertPoint(loop_bb);
  
  //execute the body
  if (body) body->codegen(module, builder);
  if (!builder.GetInsertBlock()->getTerminator()) builder.CreateBr(next_bb); //return to the start of the loop
  
  func->getBasicBlockList().push_back(next_bb);
  builder.SetInsertPoint(next_bb);

  if (after) after->codegen(module, builder);
  builder.CreateBr(pre_bb);

  func->getBasicBlockList().push_back(post_bb);
  builder.SetInsertPoint(post_bb);

  functions->scope_pop(module, builder);
  variables->scope_pop(module, builder);
  control->pop_loop();

  return NULL;
}

/** Break **/

raytrace::ast::break_statement::break_statement(control_state *control) :
  control(control)
{

}

Value *raytrace::ast::break_statement::codegen(Module *module, IRBuilder<> &builder) {
  if (!control->inside_loop()) throw runtime_error("Invalid use of 'break' outside any loop");
  BasicBlock *bb = control->post_loop();
  return builder.CreateBr(bb);
}

/** Continue **/

raytrace::ast::continue_statement::continue_statement(control_state *control) :
  control(control)
{

}

Value *raytrace::ast::continue_statement::codegen(Module *module, IRBuilder<> &builder) {
  if (!control->inside_loop()) throw runtime_error("Invalid use of 'continue' outside any loop");
  BasicBlock *bb = control->next_iter();
  return builder.CreateBr(bb);
}
