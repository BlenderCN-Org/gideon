#include "compiler/ast/expression.hpp"
#include "compiler/operations.hpp"
#include "compiler/llvm_helper.hpp"

#include <iostream>

using namespace std;
using namespace raytrace;
using namespace llvm;

/** Expression Base **/

typed_value_container ast::expression::codegen_ptr(Module *module, IRBuilder<> &builder) {
  return errors::make_error<errors::error_message>("Cannot convert expression to lvalue", 0, 0);
}

code_value ast::expression::codegen_module() {
  return errors::make_error<errors::error_message>("Cannot convert expression to module", 0, 0);
}

void ast::expression::destroy_unbound(typed_value_container &val, Module *module, IRBuilder<> &builder) {
  boost::function<codegen_void (typed_value &)> dtor = [module, &builder] (typed_value &arg) -> codegen_void {
    if (arg.get<0>().type() != value::LLVM_VALUE) return empty_type();

    type_spec t = arg.get<1>();    
    Value *val = arg.get<0>().extract_value();

    //ensure we have a pointer to this object
    Value *val_ptr = t->allocate(module, builder);
    t->store(val, val_ptr, module, builder);
    
    return t->destroy(val_ptr, module, builder);
  };

  errors::codegen_call<typed_value_container, codegen_void>(val, dtor);
}



/** Binary Expression **/

raytrace::ast::binary_expression::binary_expression(parser_state *st, const string &op, const expression_ptr &lhs, const expression_ptr &rhs,
						    unsigned int line_no, unsigned int column_no) :
  expression(st, line_no, column_no), op(op), lhs(lhs), rhs(rhs)
{
  
}

typecheck_value ast::binary_expression::typecheck() {
  typecheck_value ltype = lhs->typecheck();
  typecheck_value rtype = rhs->typecheck();

  boost::function<binop_table::op_result_value (type_spec &, type_spec &)> lookup = [this] (type_spec &left,
											    type_spec &right) -> binop_table::op_result_value {
    return state->binary_operations.find_best_operation(op, left, right, state->type_conversions);
  };

  binop_table::op_result_value op_func = errors::codegen_apply(lookup, ltype, rtype);
  
  return errors::codegen_call<binop_table::op_result_value, typecheck_value>(op_func,
									     [] (binop_table::op_result &op_func) -> typecheck_value {
									       return op_func.second.result_type;
									     });
}

typed_value_container ast::binary_expression::execute_op(binop_table::op_result_value &op_func,
							 Module *module, IRBuilder<> &builder,
							 Value* lhs_val, Value *rhs_val,
							 const type_spec &lhs_type, const type_spec &rhs_type) {
  boost::function<typed_value_container (binop_table::op_result&)> exec_op = [this, module, &builder,
									      lhs_val, rhs_val,
									      &lhs_type, &rhs_type] 
    (binop_table::op_result &op_func) -> typed_value_container {

    code_value lhs_arg = typecast(lhs_val, lhs_type, op_func.first.first, false, false, module, builder);
    code_value rhs_arg = typecast(rhs_val, rhs_type, op_func.first.second, false, false, module, builder);

    boost::function<typed_value_container (value &, value &)> check_args = [module, &builder, &op_func] (value &lval, value &rval) {
      type_spec rt = op_func.second.result_type;
      Value *result = op_func.second.codegen(lval.extract_value(),
					     rval.extract_value(),
					     module, builder);
      return typed_value(result, rt);
    };

    return errors::codegen_apply(check_args, lhs_arg, rhs_arg);
  };

  return errors::codegen_call<binop_table::op_result_value, typed_value_container>(op_func, exec_op);
}

typed_value_container ast::binary_expression::codegen(Module *module, IRBuilder<> &builder) {
  typed_value_container lhs_val = lhs->codegen(module, builder);
  typed_value_container rhs_val = rhs->codegen(module, builder);

  boost::function<typed_value_container (typed_value &, typed_value &)> exec_op = [this, module, &builder] (typed_value &left,
													    typed_value &right) -> typed_value_container {
    binop_table::op_result_value op_func = state->binary_operations.find_best_operation(op,
											left.get<1>(), right.get<1>(),
											state->type_conversions);
    return execute_op(op_func, module, builder,
		      left.get<0>().extract_value(), right.get<0>().extract_value(),
		      left.get<1>(), right.get<1>());
  };
  
  return errors::codegen_apply(exec_op, lhs_val, rhs_val);
}

/** Unary Op Expression **/

ast::unary_op_expression::unary_op_expression(parser_state *st, const string &op, const expression_ptr &arg,
					      unsigned int line_no, unsigned int column_no) :
  expression(st, line_no, column_no),
  op(op), arg(arg)
{
  
}

typecheck_value ast::unary_op_expression::typecheck() {
  typecheck_value arg_type = arg->typecheck();
  
  boost::function<unary_op_table::op_candidate_value (type_spec&)> lookup = [this] (type_spec &arg) -> unary_op_table::op_candidate_value {
    return state->unary_operations.find_best_operation(op, arg, state->type_conversions);
  };

  unary_op_table::op_candidate_value op_func = errors::codegen_call<typecheck_value, unary_op_table::op_candidate_value>(arg_type, lookup);
  
  return errors::codegen_call<unary_op_table::op_candidate_value, typecheck_value>(op_func,
										   [] (unary_op_table::op_candidate &op_func) -> typecheck_value {
										     return op_func.second.result_type;
										   });
}

typed_value_container ast::unary_op_expression::codegen(Module *module, IRBuilder<> &builder) {
  typed_value_container arg_val = arg->codegen(module, builder);

  return errors::codegen_call(arg_val, [this, module, &builder] (typed_value &arg) -> typed_value_container {
      unary_op_table::op_candidate_value op_func = state->unary_operations.find_best_operation(op, arg.get<1>(), state->type_conversions);
      return execute_op(op_func, module, builder, arg.get<0>().extract_value(), arg.get<1>());
    });
}

typed_value_container ast::unary_op_expression::execute_op(unary_op_table::op_candidate_value &op_func,
							   Module *module, IRBuilder<> &builder,
							   Value* arg_val, type_spec &arg_type) {
  boost::function<typed_value_container (unary_op_table::op_candidate&)> exec_op = [this, module, &builder, arg_val, &arg_type] 
    (unary_op_table::op_candidate &op_func) -> typed_value_container {
    type_spec rt = op_func.second.result_type;
    code_value converted = typecast(arg_val, arg_type, op_func.first, false, false, module, builder);

    return errors::codegen_call<code_value, typed_value_container>(converted, [&rt, &op_func, module, &builder] (value &val) -> typed_value_container {
	Value *result = op_func.second.codegen(val.extract_value(), module, builder);
	return typed_value(result, rt);
      });
  };

  return errors::codegen_call<unary_op_table::op_candidate_value, typed_value_container>(op_func, exec_op);
}
