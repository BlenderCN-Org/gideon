#include "compiler/ast/statement.hpp"
#include "compiler/llvm_helper.hpp"

using namespace std;
using namespace raytrace;
using namespace llvm;

/* Expression Statement */

raytrace::ast::expression_statement::expression_statement(parser_state *st, const expression_ptr &expr) :
  statement(st), expr(expr)
{

}

codegen_value raytrace::ast::expression_statement::codegen(Module *module, IRBuilder<> &builder) {
  typed_value_container result = expr->codegen_safe(module, builder);
  
  boost::function<codegen_value (typed_value &val)> op = [this, module, &builder] (typed_value &val) -> codegen_value {
    type_spec t = val.get<1>();
    if (!expr->bound() && (!t->llvm_type()->isVoidTy())) {
      //nobody captured this expression, destroy it
      Value *ptr = CreateEntryBlockAlloca(builder, t->llvm_type(), "uncaptured_tmp");
      builder.CreateStore(val.get<0>(), ptr, false);
      t->destroy(ptr, module, builder);
    }
    return nullptr;
  };

  return errors::codegen_call<typed_value_container, codegen_value>(result, op);
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
  push_scope();

  typedef errors::argument_value_join<codegen_void>::result_value_type arg_val_type;
  boost::function<codegen_value (arg_val_type &)> add_null_value = [] (arg_val_type &) -> codegen_value { return nullptr; };

  codegen_void rt = statements.codegen(module, builder);
  
  pop_scope(module, builder);
  codegen_value result = errors::codegen_call_args(add_null_value, rt);
  return result;
}
