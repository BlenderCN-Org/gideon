#include "compiler/ast/control.hpp"

using namespace std;
using namespace raytrace;
using namespace llvm;

/* If-Else Condition */

raytrace::ast::conditional_statement::conditional_statement(parser_state *st, const expression_ptr &cond,
							    const statement_ptr &if_branch, const statement_ptr &else_branch) :
  statement(st),
  cond(cond), if_branch(if_branch), else_branch(else_branch)
{

}

codegen_value raytrace::ast::conditional_statement::codegen(Module *module, IRBuilder<> &builder) {
  typecheck_value t = cond->typecheck_safe();
  t = errors::codegen_call(t,
			   [this] (type_spec &t) -> typecheck_value {
			     if (*t != *state->types["bool"]) return compile_error("Condition must be a boolean expression");
			     return t;
			   });
  
  codegen_value cond_val = cond->codegen(module, builder);
  boost::function<codegen_value (typed_value &)> op = [this, module, &builder] (typed_value &cond) -> codegen_value {
    Value *cond_val = cond.get<0>();
    Function *func = builder.GetInsertBlock()->getParent();
    
    BasicBlock *if_bb = BasicBlock::Create(getGlobalContext(), "if_bb", func);
    BasicBlock *else_bb = BasicBlock::Create(getGlobalContext(), "else_bb");
    BasicBlock *merge_bb = BasicBlock::Create(getGlobalContext(), "merge_bb");
    
    builder.CreateCondBr(cond_val, if_bb, else_bb);
    
    //generate code for if-block
    builder.SetInsertPoint(if_bb);
    
    codegen_value if_val = (if_branch ? if_branch->codegen(module, builder) : nullptr);
    if (!builder.GetInsertBlock()->getTerminator()) builder.CreateBr(merge_bb); //only go to the merge block if we don't return
    
    if_bb = builder.GetInsertBlock();
    
    //generate code for else-block
    func->getBasicBlockList().push_back(else_bb);
    builder.SetInsertPoint(else_bb);
    
    codegen_value else_val = (else_branch ? else_branch->codegen(module, builder) : nullptr);
    if (!builder.GetInsertBlock()->getTerminator()) builder.CreateBr(merge_bb);
    
    else_bb = builder.GetInsertBlock();
    
    //merge block
    func->getBasicBlockList().push_back(merge_bb);
    builder.SetInsertPoint(merge_bb);

    typedef errors::argument_value_join<codegen_value, codegen_value>::result_value_type arg_val_type;
    boost::function<codegen_value (arg_val_type &)> check_branches = [cond_val] (arg_val_type &) -> codegen_value { return cond_val; };
    return errors::codegen_call_args(check_branches, if_val, else_val);
  };
  
  return errors::codegen_call_args(op, cond_val, t);
}

/* For Loop Statement */

raytrace::ast::for_loop_statement::for_loop_statement(parser_state *st,
						      const statement_ptr &init,
						      const expression_ptr &cond,
						      const expression_ptr &after,
						      const statement_ptr &body) :
  statement(st), init(init), body(body), cond(cond), after(after)
{

}

codegen_value raytrace::ast::for_loop_statement::codegen(Module *module, IRBuilder<> &builder) {
  state->variables.scope_push();
  state->functions.scope_push();

  Function *func = builder.GetInsertBlock()->getParent();
  BasicBlock *pre_bb = BasicBlock::Create(getGlobalContext(), "pre_bb", func);
  BasicBlock *loop_bb = BasicBlock::Create(getGlobalContext(), "loop_bb");
  BasicBlock *next_bb = BasicBlock::Create(getGlobalContext(), "next_bb");
  BasicBlock *post_bb = BasicBlock::Create(getGlobalContext(), "post_bb");

  state->control.push_loop(post_bb, next_bb);

  //initialize the loop
  codegen_value init_val = (init ? init->codegen(module, builder) : nullptr);

  //jump into the loop condition check
  builder.CreateBr(pre_bb);
  builder.SetInsertPoint(pre_bb);

  //evaluate and ensure that the loop condition is a bool
  typecheck_value cond_type = cond->typecheck_safe();
  codegen_value cond_test = cond->codegen(module, builder);
  boost::function<codegen_value (typed_value &)> bool_check = [this, &builder, loop_bb, post_bb] (typed_value &arg) -> codegen_value {
    if (*arg.get<1>() != *state->types["bool"]) return compile_error("Condition must be a boolean expression");

    builder.CreateCondBr(arg.get<0>(), loop_bb, post_bb);
    return arg.get<0>();
  };

  cond_test = errors::codegen_call_args(bool_check, cond_test, cond_type);

  func->getBasicBlockList().push_back(loop_bb);
  builder.SetInsertPoint(loop_bb);
    
  //execute the body
  codegen_value body_val = (body ? body->codegen(module, builder) : nullptr);
  if (!builder.GetInsertBlock()->getTerminator()) builder.CreateBr(next_bb); //return to the start of the loop
  
  func->getBasicBlockList().push_back(next_bb);
  builder.SetInsertPoint(next_bb);
  
  codegen_value after_val = (after ? after->codegen(module, builder) : nullptr);
  builder.CreateBr(pre_bb);
  
  func->getBasicBlockList().push_back(post_bb);
  builder.SetInsertPoint(post_bb);
  
  codegen_void fpop = state->functions.scope_pop(module, builder);
  codegen_void vpop = state->variables.scope_pop(module, builder);
  state->control.pop_loop();

  //merge all the generated values to propagate any errors
  typedef raytrace::errors::argument_value_join<codegen_value, codegen_value, codegen_value, codegen_value, codegen_void, codegen_void>::result_value_type arg_val_type;
  boost::function<codegen_value (arg_val_type &)> final_check = [] (arg_val_type &) { return nullptr; };
  return errors::codegen_call_args(final_check, init_val, cond_test, body_val, after_val, fpop, vpop);
}

/** Break **/

raytrace::ast::break_statement::break_statement(parser_state *st) :
  statement(st)
{

}

codegen_value raytrace::ast::break_statement::codegen(Module *module, IRBuilder<> &builder) {
  if (!state->control.inside_loop()) return compile_error("Invalid use of 'break' outside any loop");
  BasicBlock *bb = state->control.post_loop();
  return builder.CreateBr(bb);
}

/** Continue **/

raytrace::ast::continue_statement::continue_statement(parser_state *st) :
  statement(st)
{

}

codegen_value raytrace::ast::continue_statement::codegen(Module *module, IRBuilder<> &builder) {
  if (!state->control.inside_loop()) return compile_error("Invalid use of 'continue' outside any loop");
  BasicBlock *bb = state->control.next_iter();
  return builder.CreateBr(bb);
}
