#include "compiler/ast/expression.hpp"
#include "compiler/operations.hpp"
#include "compiler/llvm_helper.hpp"

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

typed_value_container ast::expression::codegen_safe(llvm::Module *module, llvm::IRBuilder<> &builder) {
  typecheck_value type = typecheck_safe();
  codegen_value val = codegen(module, builder);
  return errors::combine_arg_list(val, type);
}

void ast::expression::destroy_unbound(typecheck_value &type, codegen_value &val, Module *module, IRBuilder<> &builder) {
  typedef raytrace::errors::argument_value_join<codegen_value, typecheck_value>::result_value_type arg_val_type;
  boost::function<codegen_void (arg_val_type &)> dtor = [module, &builder] (arg_val_type &arg) -> codegen_void {
    type_spec t = arg.get<1>();
    Value *val = arg.get<0>();

    //ensure we have a pointer to this object
    Value *val_ptr = CreateEntryBlockAlloca(builder, t->llvm_type(), "dtor_tmp");
    builder.CreateStore(val, val_ptr, false);

    return t->destroy(val_ptr, module, builder);
  };

  errors::codegen_call_args(dtor, val, type);
}



/** Binary Expression **/

raytrace::ast::binary_expression::binary_expression(parser_state *st, const string &op, const expression_ptr &lhs, const expression_ptr &rhs,
						    unsigned int line_no, unsigned int column_no) :
  expression(st, line_no, column_no), op(op), lhs(lhs), rhs(rhs)
{
  
}

binop_table::op_result_value ast::binary_expression::get_op() {
  typecheck_value lt = lhs->typecheck_safe();
  typecheck_value rt = rhs->typecheck_safe();

  typedef errors::argument_value_join<typecheck_value, typecheck_value>::result_value_type arg_val_type;
  boost::function<binop_table::op_result_value (arg_val_type &)> find_op = [this] (arg_val_type &types) -> binop_table::op_result_value {
    return state->binary_operations.find_best_operation(op, types.get<0>(), types.get<1>());
  };

  return errors::codegen_call_args(find_op, lt, rt);
}

raytrace::type_spec raytrace::ast::binary_expression::typecheck() {
  binop_table::op_result_value op_to_use = get_op();
  binop_table::op_result op_val = boost::apply_visitor(errors::return_or_throw<binop_table::op_result>(), op_to_use);
  return op_val.second.result_type;
}

codegen_value raytrace::ast::binary_expression::codegen(Module *module, IRBuilder<> &builder) {
  binop_table::op_result_value op_to_use = get_op();
  codegen_value l_val = lhs->codegen(module, builder);
  codegen_value r_val = rhs->codegen(module, builder);
  
  typedef errors::argument_value_join<binop_table::op_result_value, codegen_value, codegen_value>::result_value_type arg_val_type;
  boost::function<codegen_value (arg_val_type &)> exec_op = [module, &builder] (arg_val_type &arg) -> codegen_value {
    binop_table::op_codegen &op_func = arg.get<0>().second.codegen;
    return op_func(arg.get<1>(), arg.get<2>(), module, builder);
  };
  return errors::codegen_call_args(exec_op, op_to_use, l_val, r_val);
}

