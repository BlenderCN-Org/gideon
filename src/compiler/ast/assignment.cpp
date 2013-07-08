#include "compiler/ast/assignment.hpp"

using namespace std;
using namespace raytrace;
using namespace llvm;


/** Assignment **/

raytrace::ast::assignment::assignment(parser_state *st, const expression_ptr &lhs, const expression_ptr &rhs,
				      unsigned int line_no, unsigned int column_no) :
  expression(st, line_no, column_no), lhs(lhs), rhs(rhs)
{
  
}

typecheck_value ast::assignment::typecheck() {
  return lhs->typecheck();
}

pair<typed_value_container, typed_value_container> ast::assignment::get_value_and_pointer(Module *module, IRBuilder<> &builder) {
  typed_value_container ptr = lhs->codegen_ptr(module, builder);
  typed_value_container value = rhs->codegen(module, builder);

  typedef raytrace::errors::argument_value_join<typed_value_container, typed_value_container>::result_value_type arg_val_type;  
  boost::function<code_value (arg_val_type&)> cast_rhs = [this, &value, module, &builder] (arg_val_type &args) -> code_value {
    type_spec lt = errors::get<0>(args).get<1>();
    return typecast(value, lt, rhs->bound(), true, module, builder);
  };
  
  code_value cast_value = errors::codegen_call_args(cast_rhs, ptr, value);
  
  typedef raytrace::errors::argument_value_join<typed_value_container, code_value>::result_value_type assign_arg_type;  
  boost::function<typed_value_container (assign_arg_type &)> op = [this, module, &builder] (assign_arg_type &args) -> typed_value_container {
    type_spec lt = errors::get<0>(args).get<1>();
    Value *new_val = errors::get<1>(args).extract_value();
    
    //now destroy the old value
    Value *ptr = errors::get<0>(args).get<0>().extract_value();
    lt->destroy(ptr, module, builder);
    
    lt->store(new_val, ptr, module, builder);
    return typed_value(new_val, lt);
  };
  
  return make_pair(errors::codegen_call_args(op, ptr, cast_value), ptr);
}

typed_value_container ast::assignment::codegen(Module *module, IRBuilder<> &builder) {
  pair<typed_value_container, typed_value_container> assigned = get_value_and_pointer(module, builder);
  return assigned.first;
}

typed_value_container ast::assignment::codegen_ptr(Module *module, IRBuilder<> &builder) {
  pair<typed_value_container, typed_value_container> assigned = get_value_and_pointer(module, builder);
  
  //if any errors occurred we want to propagate only the result of the assignment
  return errors::codegen_call<typed_value_container>(assigned.first,
						     [&assigned] (typed_value &arg) -> typed_value_container {
						       return assigned.second;
						     });
}

/** Assignment Operator **/

ast::assignment_operator::assignment_operator(parser_state *st, const string &op, bool return_prior,
					      const expression_ptr &lhs, const expression_ptr &rhs,
					      unsigned int line_no, unsigned int column_no) :
  expression(st, line_no, column_no),
  op(op),
  lhs(lhs), rhs(rhs),
  return_prior(return_prior)
{
  
}

typecheck_value ast::assignment_operator::typecheck() {
  return lhs->typecheck();
}

typed_value_container ast::assignment_operator::codegen(Module *module, IRBuilder<> &builder) {
  auto assigned = get_value_and_pointer(module, builder);
  return assigned.first;
}

typed_value_container ast::assignment_operator::codegen_ptr(Module *module, IRBuilder<> &builder) {
  auto assigned = get_value_and_pointer(module, builder);
  
  //if any errors occurred we want to propagate only the result of the assignment
  return errors::codegen_call<typed_value_container>(assigned.first,
						     [&assigned] (typed_value &arg) -> typed_value_container {
						       return assigned.second;
						     });
}

pair<typed_value_container, typed_value_container> ast::assignment_operator::get_value_and_pointer(Module *module, IRBuilder<> &builder) {
  typed_value_container ptr = lhs->codegen_ptr(module, builder);
  typed_value_container rhs_val = rhs->codegen(module, builder);
  
  //lookup and evaluate the operation
  typedef errors::argument_value_join<typed_value_container, typed_value_container>::result_value_type arg_pair;
  boost::function<typed_value_container (arg_pair&)> lookup = [this, module, &builder] (arg_pair &args) -> typed_value_container {
    type_spec lhs_type = errors::get<0>(args).get<1>();
    type_spec rhs_type = errors::get<1>(args).get<1>();
    
    binop_table::op_result_value op_func = state->binary_operations.find_best_operation(op, lhs_type, rhs_type,
											state->type_conversions);
    errors::error_set_location(op_func, line_no, column_no);
    
    Value *lhs_ptr = errors::get<0>(args).get<0>().extract_value();
    Value *rhs = errors::get<1>(args).get<0>().extract_value();
    return execute_assignment(op_func, module, builder, lhs_type, rhs_type, lhs_ptr, rhs);    
  };

  return make_pair(errors::codegen_call_args(lookup, ptr, rhs_val), ptr);
}

typed_value_container ast::assignment_operator::execute_assignment(binop_table::op_result_value &op_func,
								   Module *module, IRBuilder<> &builder,
								   type_spec &lhs_type, type_spec &rhs_type,
								   Value *lhs_ptr, Value *rhs_val) {
  boost::function<typed_value_container (binop_table::op_result &)> exec = [this, module, &builder,
									    &lhs_type, &rhs_type,
									    lhs_ptr, rhs_val] (binop_table::op_result &func) -> typed_value_container {
    if (*func.second.result_type != *lhs_type) {
      //resulting value must be of the same type
      return errors::make_error<errors::type_mismatch>(func.second.result_type->name,
						       lhs_type->name, line_no, column_no);
    }

    //evaluate operation
    Value *lhs_val = lhs_type->load(lhs_ptr, module, builder);

    //cast the rhs to the appropriate type
    code_value rhs_cast = typecast(rhs_val, rhs_type,
				   func.first.second, false, !rhs->bound(), module, builder);
    
    return errors::codegen_call<code_value, typed_value_container>(rhs_cast, [&] (value &rhs_final) -> typed_value_container {
	Value *new_val = func.second.codegen(lhs_val, rhs_final.extract_value(),
					     module, builder);

	//destroy old value
	if (!return_prior) lhs_type->destroy(lhs_ptr, module, builder);
	
	//store result into ptr
	lhs_type->store(new_val, lhs_ptr, module, builder);
	
	//return new value
	Value *rt_val = (return_prior ? lhs_val : new_val);
	return typed_value(rt_val, lhs_type);
      });
  };
  
  return errors::codegen_call<binop_table::op_result_value, typed_value_container>(op_func, exec);
}
