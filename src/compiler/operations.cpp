#include "compiler/operations.hpp"

#include <stdexcept>

#include <iostream>

using namespace raytrace;
using namespace std;
using namespace llvm;

static void invalid_conversion(const string &t0, const string &t1) {
  string error = string("Invalid conversion from '") + t0 + string("' to '") + t1 + string("'");
  throw runtime_error(error);
}

static void invalid_operation(const string &op, const string &t0, const string &t1) {
  string error = string("Invalid operation '") + op + string("' on types '") + t0 + string("' and '") + t1 + string("'");
  throw runtime_error(error);
}

type_spec raytrace::get_add_result_type(const ast::expression_ptr &lhs, const ast::expression_ptr &rhs) {
  type_spec lt = lhs->typecheck();
  type_spec rt = rhs->typecheck();
  if (lt != rt) invalid_operation("+", get_type_traits(lt.t).name, get_type_traits(rt.t).name);
  
  return lt;
}

Value *raytrace::generate_add(ast::expression_ptr &lhs, ast::expression_ptr &rhs,
			      Module *module, IRBuilder<> &builder) {
  type_spec lt = lhs->typecheck();
  type_spec rt = rhs->typecheck();
  type_traits lt_t = get_type_traits(lt.t);
  type_traits rt_t = get_type_traits(rt.t);

  if (lt != rt) invalid_conversion(lt_t.name, rt_t.name);
  
  Value *lval = lhs->codegen(module, builder);
  Value *rval = rhs->codegen(module, builder);

  if (lt.t == type_code::FLOAT) {
    return builder.CreateFAdd(lval, rval, "f_add_tmp");
  }
  else if (lt.t == type_code::INT) {
    return builder.CreateAdd(lval, rval, "i_add_tmp");
  }
  else if (lt.t == type_code::FLOAT4) {
    return llvm_builtin_binop("rtl_add_v4v4", get_llvm_type(type_code::FLOAT4), lval, rval, module, builder);
  }

  invalid_operation("+", lt_t.name, rt_t.name);
  return NULL;
}

Value *raytrace::generate_less_than(ast::expression_ptr &lhs, ast::expression_ptr &rhs,
				    Module *module, IRBuilder<> &builder) {
  type_spec lt = lhs->typecheck();
  type_spec rt = rhs->typecheck();
  type_traits lt_t = get_type_traits(lt.t);
  type_traits rt_t = get_type_traits(rt.t);

  if (lt != rt) invalid_conversion(lt_t.name, rt_t.name);
  
  Value *lval = lhs->codegen(module, builder);
  Value *rval = rhs->codegen(module, builder);

  if (lt.t == type_code::FLOAT) {
    return builder.CreateFCmpOLT(lval, rval, "f_le_tmp");
  }
  else if (lt.t == type_code::INT) {
    return builder.CreateICmpSLT(lval, rval, "i_le_tmp");
  }
  
  invalid_operation("<", lt_t.name, rt_t.name);
}

Value *raytrace::llvm_builtin_binop(const string &func_name, Type *type, Value *lhs, Value *rhs,
				    Module *module, IRBuilder<> &builder) {
  Type *ptr_ty = PointerType::getUnqual(type);
  vector<Type*> arg_ty{ptr_ty, ptr_ty, ptr_ty};
  FunctionType *ft = FunctionType::get(Type::getVoidTy(getGlobalContext()),
				       ArrayRef<Type*>(arg_ty), false);
  Function *op_func = cast<Function>(module->getOrInsertFunction(func_name.c_str(), ft));
  
  Value *l_ptr = builder.CreateAlloca(type, NULL, "tmp_lhs");
  builder.CreateStore(lhs, l_ptr, false);

  Value *r_ptr = builder.CreateAlloca(type, NULL, "tmp_rhs");
  builder.CreateStore(rhs, r_ptr, false);

  Value *out_ptr = builder.CreateAlloca(type, NULL, "tmp_result");
    
  vector<Value*> args{out_ptr, l_ptr, r_ptr};
  builder.CreateCall(op_func, args);
  return builder.CreateLoad(out_ptr);
}
