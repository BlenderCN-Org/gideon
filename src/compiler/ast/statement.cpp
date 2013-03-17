#include "compiler/ast/statement.hpp"

using namespace std;
using namespace raytrace;
using namespace llvm;

/* Expression Statement */

raytrace::ast::expression_statement::expression_statement(const expression_ptr &expr) :
  statement(), expr(expr)
{

}

Value *raytrace::ast::expression_statement::codegen(Module *module, IRBuilder<> &builder) {
  return expr->codegen(module, builder);
}

/* Statement List */

raytrace::ast::statement_list::statement_list(const vector<statement_ptr> &statements) :
  statements(statements)
{
  
}

void raytrace::ast::statement_list::codegen(Module *module, IRBuilder<> &builder) {
  for (auto it = statements.begin(); it != statements.end(); it++) {
    (*it)->codegen(module, builder);
    if ((*it)->is_terminator()) return; //ignore all statements after a terminator
  }
}

/* Scoped Statements */

raytrace::ast::scoped_statement::scoped_statement(const statement_list &stmt_list,
						  var_symbol_table *variables, func_symbol_table *functions) :
  statement(),
  statements(stmt_list), variables(variables), functions(functions)
{
  
}

Value *raytrace::ast::scoped_statement::codegen(Module *module, IRBuilder<> &builder) {
  variables->scope_push();
  functions->scope_push();

  statements.codegen(module, builder);

  functions->scope_pop(module, builder);
  variables->scope_pop(module, builder); 
  return NULL;
}
