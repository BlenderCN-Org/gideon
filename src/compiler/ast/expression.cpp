#include "compiler/ast/expression.hpp"
#include "compiler/operations.hpp"

#include <iostream>

using namespace std;
using namespace raytrace;
using namespace llvm;

/** Expression Base **/

codegen_value raytrace::ast::expression::codegen_ptr(Module *module, IRBuilder<> &builder) {
  return compile_error("Cannot convert expression to lvalue");
}

typecheck_value ast::expression::typecheck_safe() {
  try { return typecheck(); }
  catch (compile_error &e) { return e; }
}

/** Binary Expression **/

raytrace::ast::binary_expression::binary_expression(parser_state *st, const string &op, const expression_ptr &lhs, const expression_ptr &rhs) :
  expression(st), op(op), lhs(lhs), rhs(rhs)
{
  
}

raytrace::type_spec raytrace::ast::binary_expression::typecheck() {
  if (op == "+") return get_add_result_type(lhs, rhs);
  if (op == "<") return state->types["bool"];
}

codegen_value raytrace::ast::binary_expression::codegen(Module *module, IRBuilder<> &builder) {
  try {
    if (op == "+") return generate_add(lhs, rhs, state->types, module, builder);
    if (op == "<") return generate_less_than(lhs, rhs, state->types, module, builder);
  }
  catch (compile_error &e) { return e; }
}

