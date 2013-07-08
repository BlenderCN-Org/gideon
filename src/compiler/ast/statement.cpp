#include "compiler/ast/statement.hpp"
#include "compiler/llvm_helper.hpp"

using namespace std;
using namespace raytrace;
using namespace llvm;

/* Expression Statement */

raytrace::ast::expression_statement::expression_statement(parser_state *st, const expression_ptr &expr,
							  unsigned int line_no, unsigned int column_no) :
  statement(st, line_no, column_no), expr(expr)
{

}

codegen_void raytrace::ast::expression_statement::codegen(Module *module, IRBuilder<> &builder) {
  typed_value_container result = expr->codegen(module, builder);
  
  boost::function<codegen_void (typed_value &val)> op = [this, module, &builder] (typed_value &val) -> codegen_void {
    type_spec t = val.get<1>();
    if (!expr->bound() && (!t->llvm_type()->isVoidTy())) {
      //nobody captured this expression, destroy it
      if (val.get<0>().type() == value::LLVM_VALUE) {
	Value *ptr = t->allocate(module, builder);
	t->store(val.get<0>().extract_value(), ptr, module, builder);
	
	t->destroy(ptr, module, builder);
      }
    }
    return empty_type();
  };

  return errors::codegen_call<typed_value_container, codegen_void>(result, op);
}

/* Statement List */

raytrace::ast::statement_list::statement_list(const vector<statement_ptr> &statements) :
  statements(statements)
{
  
}

codegen_void raytrace::ast::statement_list::codegen(Module *module, IRBuilder<> &builder) {
  codegen_void result = empty_type();

  for (auto it = statements.begin(); it != statements.end(); it++) {
    codegen_void val = (*it)->codegen(module, builder);
    result = errors::merge_void_values(result, val);
    
    if ((*it)->is_terminator()) break; //ignore all statements after a terminator
  }
  
  return result;
}

/* Scoped Statements */

raytrace::ast::scoped_statement::scoped_statement(parser_state *st, const statement_list &stmt_list,
						  unsigned int line_no, unsigned int column_no) :
  statement(st, line_no, column_no), statements(stmt_list)
{
  
}

codegen_void raytrace::ast::scoped_statement::codegen(Module *module, IRBuilder<> &builder) {
  push_scope();

  typedef errors::argument_value_join<codegen_void>::result_value_type arg_val_type;
  boost::function<codegen_void (arg_val_type &)> add_null_value = [] (arg_val_type &) -> codegen_void { return empty_type(); };

  codegen_void rt = statements.codegen(module, builder);
  
  pop_scope(module, builder);
  return errors::codegen_call_args(add_null_value, rt);
}
