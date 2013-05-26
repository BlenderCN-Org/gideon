#include "compiler/ast/assignment.hpp"

using namespace std;
using namespace raytrace;
using namespace llvm;


/** Assignment **/

raytrace::ast::assignment::assignment(parser_state *st, const expression_ptr &lhs, const expression_ptr &rhs) :
  expression(st), lhs(lhs), rhs(rhs)
{
  
}

typecheck_value ast::assignment::typecheck() {
  return lhs->typecheck();
}

pair<typed_value_container, typed_value_container> ast::assignment::get_value_and_pointer(Module *module, IRBuilder<> &builder) {
  typed_value_container ptr = lhs->codegen_ptr(module, builder);
  typed_value_container value = rhs->codegen(module, builder);
  
  typedef raytrace::errors::argument_value_join<typed_value_container, typed_value_container>::result_value_type arg_val_type;  
  boost::function<typed_value_container (arg_val_type &)> op = [this, module, &builder] (arg_val_type &args) -> typed_value_container {
    type_spec lt = args.get<0>().get<1>();
    type_spec rt = args.get<1>().get<1>();
    int cost;
    if (!rt->can_cast_to(*lt, cost)) {
      stringstream err;
      err << "Cannot convert value of type '" << rt->name << "' to variable of type '" << lt->name << "' in assignment.";
      return errors::make_error<errors::error_message>(err.str(), line_no, column_no);
    }
    
    Value *new_val = args.get<1>().get<0>().extract_value();
    
    //if the rhs is already bound to a variable, make a copy
    if (rhs->bound()) new_val = rt->copy(new_val, module, builder);

    //now destroy the old value
    Value *ptr = args.get<0>().get<0>().extract_value();
    lt->destroy(ptr, module, builder);
    
    lt->store(new_val, ptr, module, builder);
    return typed_value(new_val, lt);
  };
  
  return make_pair(errors::codegen_call_args(op, ptr, value), ptr);
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

ast::assignment_operator::assignment_operator(parser_state *st, const string &op,
					      const expression_ptr &lhs, const expression_ptr &rhs,
					      unsigned int line_no, unsigned int column_no) :
  expression(st, line_no, column_no),
  op(op),
  lhs(lhs), rhs(rhs)
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
    type_spec lhs_type = args.get<0>().get<1>();
    type_spec rhs_type = args.get<1>().get<1>();
    
    binop_table::op_result_value op_func = state->binary_operations.find_best_operation(op, lhs_type, rhs_type);
    
    Value *lhs_ptr = args.get<0>().get<0>().extract_value();
    Value *rhs = args.get<1>().get<0>().extract_value();
    return execute_assignment(op_func, module, builder, lhs_type, lhs_ptr, rhs);    
  };

  return make_pair(errors::codegen_call_args(lookup, ptr, rhs_val), ptr);
}

typed_value_container ast::assignment_operator::execute_assignment(binop_table::op_result_value &op_func,
								   Module *module, IRBuilder<> &builder,
								   type_spec &lhs_type,
								   Value *lhs_ptr, Value *rhs_val) {
  boost::function<typed_value_container (binop_table::op_result &)> exec = [this, module, &builder,
									    &lhs_type, lhs_ptr, rhs_val] (binop_table::op_result &func) -> typed_value_container {
    if (*func.second.result_type != *lhs_type) {
      //resulting value must be of the same type
      return errors::make_error<errors::type_mismatch>(func.second.result_type->name,
						       lhs_type->name, line_no, column_no);
    }

    //evaluate operation
    Value *lhs_val = lhs_type->load(lhs_ptr, module, builder);
    Value *new_val = func.second.codegen(lhs_val, rhs_val,
					 module, builder);

    //destroy old value
    lhs_type->destroy(lhs_ptr, module, builder);

    //store result into ptr
    lhs_type->store(new_val, lhs_ptr, module, builder);
    
    //return new value
    return typed_value(new_val, lhs_type);
  };
  
  return errors::codegen_call<binop_table::op_result_value, typed_value_container>(op_func, exec);
}
