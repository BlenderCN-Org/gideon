#include "compiler/ast/statement.hpp"

using namespace std;
using namespace raytrace;
using namespace llvm;

/* Expression Statement */

raytrace::ast::expression_statement::expression_statement(parser_state *st, const expression_ptr &expr) :
  statement(st), expr(expr)
{

}

codegen_value raytrace::ast::expression_statement::codegen(Module *module, IRBuilder<> &builder) {
  return expr->codegen(module, builder);
}

/* Statement List */

raytrace::ast::statement_list::statement_list(const vector<statement_ptr> &statements) :
  statements(statements)
{
  
}

codegen_void raytrace::ast::statement_list::codegen(Module *module, IRBuilder<> &builder) {
  codegen_void result = nullptr;

  for (auto it = statements.begin(); it != statements.end(); it++) {
    codegen_value val = (*it)->codegen(module, builder);
    codegen_void stmt_rt = errors::codegen_ignore_value(val);
    result = errors::merge_void_values(result, stmt_rt);
    
    if ((*it)->is_terminator()) break; //ignore all statements after a terminator
  }
  
  return result;
}

/* Scoped Statements */

raytrace::ast::scoped_statement::scoped_statement(parser_state *st, const statement_list &stmt_list) :
  statement(st), statements(stmt_list)
{
  
}

codegen_value raytrace::ast::scoped_statement::codegen(Module *module, IRBuilder<> &builder) {
  state->variables.scope_push();
  state->functions.scope_push();

  typedef errors::argument_value_join<codegen_void, codegen_void, codegen_void>::result_value_type arg_val_type;
  boost::function<codegen_value (arg_val_type &)> add_null_value = [] (arg_val_type &) -> codegen_value { return nullptr; };

  codegen_void rt = statements.codegen(module, builder);
  
  codegen_void fpop = state->functions.scope_pop(module, builder);
  codegen_void vpop = state->variables.scope_pop(module, builder);

  return errors::codegen_call_args(add_null_value, rt, vpop, fpop);
}
