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
    Value *val_ptr = CreateEntryBlockAlloca(builder, t->llvm_type(), "dtor_tmp");
    builder.CreateStore(val, val_ptr, false);

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

  typedef errors::argument_value_join<typecheck_value, typecheck_value>::result_value_type typecheck_pair;
  boost::function<binop_table::op_result_value (typecheck_pair&)> lookup = [this] (typecheck_pair &args) -> binop_table::op_result_value {
    return state->binary_operations.find_best_operation(op, args.get<0>(), args.get<1>());
  };

  binop_table::op_result_value op_func = errors::codegen_call_args(lookup, ltype, rtype);
  
  return errors::codegen_call<binop_table::op_result_value, typecheck_value>(op_func,
									     [] (binop_table::op_result &op_func) -> typecheck_value {
									       return op_func.second.result_type;
									     });
}

typed_value_container ast::binary_expression::execute_op(binop_table::op_result_value &op_func,
							 Module *module, IRBuilder<> &builder,
							 Value* lhs_val, Value *rhs_val) {
  boost::function<typed_value_container (binop_table::op_result&)> exec_op = [module, &builder, lhs_val, rhs_val] 
    (binop_table::op_result &op_func) -> typed_value_container {
    type_spec rt = op_func.second.result_type;
    Value *result = op_func.second.codegen(lhs_val, rhs_val, module, builder);
    return typed_value(result, rt);
  };

  return errors::codegen_call<binop_table::op_result_value, typed_value_container>(op_func, exec_op);
}

typed_value_container ast::binary_expression::codegen(Module *module, IRBuilder<> &builder) {
  typed_value_container lhs_val = lhs->codegen(module, builder);
  typed_value_container rhs_val = rhs->codegen(module, builder);
  
  typedef errors::argument_value_join<typed_value_container, typed_value_container>::result_value_type binary_op_args;
  boost::function<typed_value_container (binary_op_args &)> exec_op = [this, module, &builder] (binary_op_args &args) -> typed_value_container {
    binop_table::op_result_value op_func = state->binary_operations.find_best_operation(op,
											args.get<0>().get<1>(), args.get<1>().get<1>());
    return execute_op(op_func, module, builder,
		      args.get<0>().get<0>().extract_value(), args.get<1>().get<0>().extract_value());
  };
  return errors::codegen_call_args(exec_op, lhs_val, rhs_val);
}
