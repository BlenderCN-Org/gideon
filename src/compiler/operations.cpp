#include "compiler/operations.hpp"
#include "compiler/llvm_helper.hpp"

#include <stdexcept>

#include <iostream>

using namespace raytrace;
using namespace std;
using namespace llvm;

static compile_error invalid_conversion(const string &t0, const string &t1) {
  string error = string("Invalid conversion from '") + t0 + string("' to '") + t1 + string("'");
  return runtime_error(error);
}

static compile_error invalid_operation(const string &op, const string &t0, const string &t1) {
  string error = string("Invalid operation '") + op + string("' on types '") + t0 + string("' and '") + t1 + string("'");
  return runtime_error(error);
}

type_spec raytrace::get_add_result_type(const ast::expression_ptr &lhs, const ast::expression_ptr &rhs) {
  type_spec lt = lhs->typecheck();
  type_spec rt = rhs->typecheck();
  if (*lt != *rt) throw invalid_operation("+", lt->name, rt->name);
  
  return lt;
}

type_spec raytrace::get_sub_result_type(const ast::expression_ptr &lhs, const ast::expression_ptr &rhs) {
  type_spec lt = lhs->typecheck();
  type_spec rt = rhs->typecheck();
  if (*lt != *rt) throw invalid_operation("-", lt->name, rt->name);
  
  return lt;
}

void destroy_unbound_arg(ast::expression_ptr &expr, type_spec t, codegen_value &val,
			 Module *module, IRBuilder<> &builder) {
  if (!expr->bound()) {
    typecheck_value safe = t;
    ast::expression::destroy_unbound(safe, val, module, builder);
  }
}

codegen_value raytrace::generate_add(ast::expression_ptr &lhs, ast::expression_ptr &rhs,
			      type_table &types,
			      Module *module, IRBuilder<> &builder) {
  type_spec lt = lhs->typecheck();
  type_spec rt = rhs->typecheck();
  
  if (*lt != *rt) return invalid_conversion(lt->name, rt->name);
  
  codegen_value lval = lhs->codegen(module, builder);
  codegen_value rval = rhs->codegen(module, builder);

  codegen_value result = lt->op_add(module, builder, lval, rval);

  destroy_unbound_arg(lhs, lt, lval, module, builder);
  destroy_unbound_arg(rhs, rt, rval, module, builder);

  return result;
}

codegen_value raytrace::generate_sub(ast::expression_ptr &lhs, ast::expression_ptr &rhs, type_table &types,
				     Module *module, IRBuilder<> &builder) {
  typecheck_value lt = lhs->typecheck_safe();
  typecheck_value rt = rhs->typecheck_safe();
  
  typedef errors::argument_value_join<typecheck_value, typecheck_value>::result_value_type arg_val_type;
  boost::function<codegen_value (arg_val_type &)> op = [&lhs, &rhs, module, &builder] (arg_val_type &arg) -> codegen_value {
    type_spec lt = arg.get<0>();
    type_spec rt = arg.get<1>();
    
    if (*lt != *rt) return invalid_conversion(lt->name, rt->name);

    codegen_value lval = lhs->codegen(module, builder);
    codegen_value rval = rhs->codegen(module, builder);

    codegen_value result = lt->op_sub(module, builder, lval, rval);
    destroy_unbound_arg(lhs, lt, lval, module, builder);
    destroy_unbound_arg(rhs, rt, rval, module, builder);
    
    return result; 
  };

  return errors::codegen_call_args(op, lt, rt);
}

codegen_value raytrace::generate_mul(ast::expression_ptr &lhs, ast::expression_ptr &rhs, type_table &types,
				     Module *module, IRBuilder<> &builder) {
  typecheck_value lt = lhs->typecheck_safe();
  typecheck_value rt = rhs->typecheck_safe();
  
  typedef errors::argument_value_join<typecheck_value, typecheck_value>::result_value_type arg_val_type;
  boost::function<codegen_value (arg_val_type &)> op = [&lhs, &rhs, module, &builder] (arg_val_type &arg) -> codegen_value {
    type_spec lt = arg.get<0>();
    type_spec rt = arg.get<1>();
    
    if (*lt != *rt) return invalid_conversion(lt->name, rt->name);

    codegen_value lval = lhs->codegen(module, builder);
    codegen_value rval = rhs->codegen(module, builder);

    codegen_value result = lt->op_mul(module, builder, lval, rval);
    destroy_unbound_arg(lhs, lt, lval, module, builder);
    destroy_unbound_arg(rhs, rt, rval, module, builder);
    
    return result;
  };
  
  return errors::codegen_call_args(op, lt, rt);
}

codegen_value raytrace::generate_div(ast::expression_ptr &lhs, ast::expression_ptr &rhs, type_table &types,
				     Module *module, IRBuilder<> &builder) {
  typecheck_value lt = lhs->typecheck_safe();
  typecheck_value rt = rhs->typecheck_safe();
  
  typedef errors::argument_value_join<typecheck_value, typecheck_value>::result_value_type arg_val_type;
  boost::function<codegen_value (arg_val_type &)> op = [&lhs, &rhs, module, &builder] (arg_val_type &arg) -> codegen_value {
    type_spec lt = arg.get<0>();
    type_spec rt = arg.get<1>();
    
    if (*lt != *rt) return invalid_conversion(lt->name, rt->name);

    codegen_value lval = lhs->codegen(module, builder);
    codegen_value rval = rhs->codegen(module, builder);

    codegen_value result = lt->op_div(module, builder, lval, rval);
    destroy_unbound_arg(lhs, lt, lval, module, builder);
    destroy_unbound_arg(rhs, rt, rval, module, builder);
    
    return result;
  };

  return errors::codegen_call_args(op, lt, rt);
}

codegen_value raytrace::generate_less_than(ast::expression_ptr &lhs, ast::expression_ptr &rhs,
				    type_table &types,
				    Module *module, IRBuilder<> &builder) {
  type_spec lt = lhs->typecheck();
  type_spec rt = rhs->typecheck();
  
  if (*lt != *rt) return invalid_conversion(lt->name, rt->name);
  
  codegen_value lval = lhs->codegen(module, builder);
  codegen_value rval = rhs->codegen(module, builder);

  codegen_value result = lt->op_less(module, builder, lval, rval);
  destroy_unbound_arg(lhs, lt, lval, module, builder);
  destroy_unbound_arg(rhs, rt, rval, module, builder);
  
  return result;
}

codegen_value raytrace::llvm_builtin_binop(const string &func_name, Type *type, Value *lhs, Value *rhs,
					   Module *module, IRBuilder<> &builder) {
  Type *ptr_ty = PointerType::getUnqual(type);
  vector<Type*> arg_ty{ptr_ty, ptr_ty, ptr_ty};
  FunctionType *ft = FunctionType::get(Type::getVoidTy(getGlobalContext()),
				       ArrayRef<Type*>(arg_ty), false);
  Function *op_func = cast<Function>(module->getOrInsertFunction(func_name.c_str(), ft));
  
  Value *l_ptr = CreateEntryBlockAlloca(builder, type, "tmp_lhs");
  builder.CreateStore(lhs, l_ptr, false);

  Value *r_ptr = CreateEntryBlockAlloca(builder, type, "tmp_rhs");
  builder.CreateStore(rhs, r_ptr, false);

  Value *out_ptr = CreateEntryBlockAlloca(builder, type, "tmp_result");
    
  vector<Value*> args{out_ptr, l_ptr, r_ptr};
  builder.CreateCall(op_func, args);
  return builder.CreateLoad(out_ptr);
}
