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

codegen_value raytrace::generate_add(ast::expression_ptr &lhs, ast::expression_ptr &rhs,
			      type_table &types,
			      Module *module, IRBuilder<> &builder) {
  type_spec lt = lhs->typecheck();
  type_spec rt = rhs->typecheck();
  
  if (*lt != *rt) return invalid_conversion(lt->name, rt->name);
  
  codegen_value lval = lhs->codegen(module, builder);
  codegen_value rval = rhs->codegen(module, builder);

  return lt->op_add(module, builder, lval, rval);
}

codegen_value raytrace::generate_less_than(ast::expression_ptr &lhs, ast::expression_ptr &rhs,
				    type_table &types,
				    Module *module, IRBuilder<> &builder) {
  type_spec lt = lhs->typecheck();
  type_spec rt = rhs->typecheck();
  
  if (*lt != *rt) return invalid_conversion(lt->name, rt->name);
  
  codegen_value lval = lhs->codegen(module, builder);
  codegen_value rval = rhs->codegen(module, builder);
  return lt->op_less(module, builder, lval, rval);

  /*typedef raytrace::errors::argument_value_join<codegen_value, codegen_value>::result_value_type arg_val_type;

  codegen_value lval = lhs->codegen(module, builder);
  codegen_value rval = rhs->codegen(module, builder);

  boost::function<codegen_value (arg_val_type &val)> op;

  if (*lt == *types["float"]) {
    op = [&builder] (arg_val_type &val) -> codegen_value { return builder.CreateFCmpOLT(val.get<0>(), val.get<1>(), "f_le_tmp"); };
  }
  else if (*lt == *types["int"]) {
    op = [&builder] (arg_val_type &val) -> codegen_value { return builder.CreateICmpSLT(val.get<0>(), val.get<1>(), "i_le_tmp"); };
  }
  else return invalid_operation("<", lt->name, rt->name);

  return errors::codegen_call_args(op, lval, rval);*/
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
