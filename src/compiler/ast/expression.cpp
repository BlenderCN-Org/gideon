#include "compiler/ast/expression.hpp"
#include "compiler/operations.hpp"

#include <iostream>

using namespace std;
using namespace raytrace::ast;
using namespace llvm;

/** Expression Base **/

Value *raytrace::ast::expression::codegen_ptr(Module *module, IRBuilder<> &builder) {
  throw runtime_error("Cannot convert expression to lvalue");
}

/** Binary Expression **/

raytrace::ast::binary_expression::binary_expression(const string &op, const expression_ptr &lhs, const expression_ptr &rhs) :
  expression(), op(op), lhs(lhs), rhs(rhs)
{
  
}

raytrace::type_spec raytrace::ast::binary_expression::typecheck() {
  if (op == "+") return get_add_result_type(lhs, rhs);
  if (op == "<") return { type_code::BOOL };
}

Value *raytrace::ast::binary_expression::codegen(Module *module, IRBuilder<> &builder) {
  if (op == "+") return generate_add(lhs, rhs, module, builder);
  if (op == "<") return generate_less_than(lhs, rhs, module, builder);
}

