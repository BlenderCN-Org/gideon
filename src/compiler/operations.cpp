/*

  Copyright 2013 Curtis Andrus

  This file is part of Gideon.

  Gideon is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  Gideon is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with Gideon.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "compiler/operations.hpp"
#include "compiler/type_conversion.hpp"
#include "compiler/llvm_helper.hpp"

#include <stdexcept>

#include <iostream>

using namespace raytrace;
using namespace std;
using namespace llvm;

/** Binary Operation Table **/

binop_table::op_candidate_vector binop_table::find_operation(const string &op, const type_spec &lhs, const type_spec &rhs) const {
  op_candidate_vector candidates;
  op_type args = make_pair(lhs, rhs);

  auto table_it = operations.find(op);
  if (table_it != operations.end()) {
    auto op_it = table_it->second.find(args);
    if (op_it != table_it->second.end()) candidates.push_back(*op_it);
  }

  return candidates;
}

binop_table::op_result_value binop_table::find_best_operation(const std::string &op, const type_spec &lhs, const type_spec &rhs,
							      const type_conversion_table &conversions) const {
  auto table_it = operations.find(op);
  if (table_it == operations.end()) return errors::make_error<errors::error_message>(string("Unsupported operation: ") + op, 0, 0);
  
  const op_codegen_table &candidates = table_it->second;

  int max_score = std::numeric_limits<int>::max();
  int best_score = max_score;
  op_candidate_vector best_list;
  
  for (auto it = candidates.begin(); it != candidates.end(); ++it) {
    int score = candidate_score(it->first, conversions, lhs, rhs);
    if (score >= max_score) continue;
    
    if (score < best_score) {
      best_score = score;
      best_list.clear();
    }
    
    if (score == best_score) best_list.push_back(*it);
  }

  if (best_list.size() == 0) return errors::make_error<errors::error_message>("Invalid operation for those types", 0, 0); //no matches
  if (best_list.size() > 1) return errors::make_error<errors::error_message>("Binary operation is ambiguous", 0, 0); //ambiguous
  return best_list[0];
}

void binop_table::add_operation(const string &op, const type_spec &lhs, const type_spec &rhs,
				const type_spec &result_type, const op_codegen &codegen) {
  op_type args = make_pair(lhs, rhs);
  op_info info{result_type, codegen};

  op_codegen_table &table = operations[op];
  table[args] = info;
}

int binop_table::candidate_score(const op_type &types, const type_conversion_table &conversions,
				 const type_spec &lhs, const type_spec &rhs) const {
  int arg_tmp;
  int l_cost, r_cost;
  if (!conversions.can_convert(lhs, types.first, arg_tmp, l_cost)) return std::numeric_limits<int>::max();
  if (!conversions.can_convert(rhs, types.second, arg_tmp, r_cost)) return std::numeric_limits<int>::max();

  return l_cost + r_cost;
}

void binop_table::initialize_standard_ops(binop_table &table, type_table &types) {
  table.add_operation("+", types["int"], types["int"], types["int"], llvm_add_i_i());
  table.add_operation("-", types["int"], types["int"], types["int"], llvm_sub_i_i());
  table.add_operation("*", types["int"], types["int"], types["int"], llvm_mul_i_i());
  table.add_operation("/", types["int"], types["int"], types["int"], llvm_div_i_i());

  table.add_operation("<", types["int"], types["int"], types["bool"], llvm_lt_i_i());
  table.add_operation(">", types["int"], types["int"], types["bool"], llvm_gt_i_i());

  table.add_operation("+", types["float"], types["float"], types["float"], llvm_add_f_f());
  table.add_operation("-", types["float"], types["float"], types["float"], llvm_sub_f_f());
  table.add_operation("*", types["float"], types["float"], types["float"], llvm_mul_f_f());
  table.add_operation("/", types["float"], types["float"], types["float"], llvm_div_f_f());

  table.add_operation("<", types["float"], types["float"], types["bool"], llvm_lt_f_f());
  table.add_operation(">", types["float"], types["float"], types["bool"], llvm_gt_f_f());

  table.add_operation("+", types["vec2"], types["vec2"], types["vec2"], llvm_add_vec_vec(2, types));
  table.add_operation("+", types["vec3"], types["vec3"], types["vec3"], llvm_add_vec_vec(3, types));
  table.add_operation("+", types["vec4"], types["vec4"], types["vec4"], llvm_add_vec_vec(4, types));

  table.add_operation("*", types["vec2"], types["vec2"], types["vec2"], llvm_mul_vec_vec(2, types));
  table.add_operation("*", types["vec3"], types["vec3"], types["vec3"], llvm_mul_vec_vec(3, types));
  table.add_operation("*", types["vec4"], types["vec4"], types["vec4"], llvm_mul_vec_vec(4, types));

  table.add_operation("-", types["vec2"], types["vec2"], types["vec2"], llvm_sub_vec_vec(2, types));
  table.add_operation("-", types["vec3"], types["vec3"], types["vec3"], llvm_sub_vec_vec(3, types));
  table.add_operation("-", types["vec4"], types["vec4"], types["vec4"], llvm_sub_vec_vec(4, types));

  table.add_operation("/", types["vec2"], types["vec2"], types["vec2"], llvm_div_vec_vec(2, types));
  table.add_operation("/", types["vec3"], types["vec3"], types["vec3"], llvm_div_vec_vec(3, types));
  table.add_operation("/", types["vec4"], types["vec4"], types["vec4"], llvm_div_vec_vec(4, types));

  table.add_operation("*", types["float"], types["vec2"], types["vec2"], llvm_scale_vec(2, false, types));
  table.add_operation("*", types["vec2"], types["float"], types["vec2"], llvm_scale_vec(2, true, types));

  table.add_operation("*", types["float"], types["vec3"], types["vec3"], llvm_scale_vec(3, false, types));
  table.add_operation("*", types["vec3"], types["float"], types["vec3"], llvm_scale_vec(3, true, types));

  table.add_operation("*", types["float"], types["vec4"], types["vec4"], llvm_scale_vec(4, false, types));
  table.add_operation("*", types["vec4"], types["float"], types["vec4"], llvm_scale_vec(4, true, types));

  table.add_operation("/", types["vec2"], types["float"], types["vec2"], llvm_inv_scale_vec(2, types));
  table.add_operation("/", types["vec3"], types["float"], types["vec3"], llvm_inv_scale_vec(3, types));
  table.add_operation("/", types["vec4"], types["float"], types["vec4"], llvm_inv_scale_vec(4, types));

  table.add_operation("+", types["string"], types["string"], types["string"],
  		      llvm_add_str_str(types["string"]->llvm_type()));

  table.add_operation("+", types["dfunc"], types["dfunc"], types["dfunc"],
		      llvm_add_dfunc_dfunc(types["dfunc"]->llvm_type()));
  table.add_operation("*", types["vec4"], types["dfunc"], types["dfunc"],
		      llvm_scale_dfunc(types["dfunc"]->llvm_type(), false, types));
  table.add_operation("*", types["dfunc"], types["vec4"], types["dfunc"],
		      llvm_scale_dfunc(types["dfunc"]->llvm_type(), false, types));

  table.add_operation("==", types["shader_flag"], types["shader_flag"], types["bool"],
		      llvm_cmp_sf_sf());
  table.add_operation("&&", types["shader_flag"], types["shader_flag"], types["bool"],
		      llvm_and_sf_sf());

  table.add_operation("+", types["shader_flag"], types["shader_flag"], types["shader_flag"],
		      llvm_add_sf_sf());
  table.add_operation("-", types["shader_flag"], types["shader_flag"], types["shader_flag"],
		      llvm_sub_sf_sf());
}

/* LLVM Code Generation Functions */

binop_table::op_codegen raytrace::llvm_add_i_i() {
  return [] (Value *lhs, Value *rhs,
	     Module *module, IRBuilder<> &builder) -> Value *{
    return builder.CreateAdd(lhs, rhs, "i_add_tmp");
  };
}

binop_table::op_codegen raytrace::llvm_sub_i_i() {
  return [] (Value *lhs, Value *rhs,
	     Module *module, IRBuilder<> &builder) -> Value *{
    return builder.CreateSub(lhs, rhs, "i_sub_tmp");
  };
}

binop_table::op_codegen raytrace::llvm_mul_i_i() {
  return [] (Value *lhs, Value *rhs,
	     Module *module, IRBuilder<> &builder) -> Value *{
    return builder.CreateMul(lhs, rhs, "i_mul_tmp");
  };
}

binop_table::op_codegen raytrace::llvm_div_i_i() {
  return [] (Value *lhs, Value *rhs,
	     Module *module, IRBuilder<> &builder) -> Value *{
    return builder.CreateSDiv(lhs, rhs, "i_div_tmp");
  };
}

binop_table::op_codegen raytrace::llvm_lt_i_i() {
  return [] (Value *lhs, Value *rhs,
	     Module *module, IRBuilder<> &builder) -> Value *{
    return builder.CreateICmpSLT(lhs, rhs, "i_lt_tmp");
  };
}

binop_table::op_codegen raytrace::llvm_gt_i_i() {
  return [] (Value *lhs, Value *rhs,
	     Module *module, IRBuilder<> &builder) -> Value *{
    return builder.CreateICmpSGT(lhs, rhs, "i_gt_tmp");
  };
}

binop_table::op_codegen raytrace::llvm_add_f_f() {
  return [] (Value *lhs, Value *rhs,
	     Module *module, IRBuilder<> &builder) -> Value *{
    return builder.CreateFAdd(lhs, rhs, "f_add_tmp");
  };
}

binop_table::op_codegen raytrace::llvm_sub_f_f() {
  return [] (Value *lhs, Value *rhs,
	     Module *module, IRBuilder<> &builder) -> Value *{
    return builder.CreateFSub(lhs, rhs, "f_sub_tmp");
  };
}

binop_table::op_codegen raytrace::llvm_mul_f_f() {
  return [] (Value *lhs, Value *rhs,
	     Module *module, IRBuilder<> &builder) -> Value *{
    return builder.CreateFMul(lhs, rhs, "f_mul_tmp");
  };
}

binop_table::op_codegen raytrace::llvm_div_f_f() {
  return [] (Value *lhs, Value *rhs,
	     Module *module, IRBuilder<> &builder) -> Value *{
    return builder.CreateFDiv(lhs, rhs, "f_div_tmp");
  };
}

binop_table::op_codegen raytrace::llvm_lt_f_f() {
  return [] (Value *lhs, Value *rhs,
	     Module *module, IRBuilder<> &builder) -> Value *{
    return builder.CreateFCmpULT(lhs, rhs, "f_lt_tmp");
  };
}

binop_table::op_codegen raytrace::llvm_gt_f_f() {
  return [] (Value *lhs, Value *rhs,
	     Module *module, IRBuilder<> &builder) -> Value *{
    return builder.CreateFCmpUGT(lhs, rhs, "f_lt_tmp");
  };
}

binop_table::op_codegen raytrace::llvm_add_vec_vec(unsigned int N, type_table &types) {
  stringstream tname;
  tname << "vec" << N;
  Type *llvm_type = types[tname.str()]->llvm_type();

  return [N, llvm_type] (Value *lhs, Value *rhs,
	     Module *module, IRBuilder<> &builder) -> Value *{
    stringstream op_ss;
    op_ss << "gd_builtin_add_v" << N << "_v" << N;
    string op_func = op_ss.str();

    return llvm_builtin_binop(op_func, llvm_type, llvm_type, llvm_type,
			      lhs, rhs, module, builder);
  };
}

binop_table::op_codegen raytrace::llvm_mul_vec_vec(unsigned int N, type_table &types) {
  stringstream tname;
  tname << "vec" << N;
  Type *llvm_type = types[tname.str()]->llvm_type();

  return [N, llvm_type] (Value *lhs, Value *rhs,
	     Module *module, IRBuilder<> &builder) -> Value *{
    stringstream op_ss;
    op_ss << "gd_builtin_mul_v" << N << "_v" << N;
    string op_func = op_ss.str();

    return llvm_builtin_binop(op_func, llvm_type, llvm_type, llvm_type,
			      lhs, rhs, module, builder);
  };
}

binop_table::op_codegen raytrace::llvm_div_vec_vec(unsigned int N, type_table &types) {
  stringstream tname;
  tname << "vec" << N;
  Type *llvm_type = types[tname.str()]->llvm_type();

  return [N, llvm_type] (Value *lhs, Value *rhs,
	     Module *module, IRBuilder<> &builder) -> Value *{
    stringstream op_ss;
    op_ss << "gd_builtin_div_v" << N << "_v" << N;
    string op_func = op_ss.str();

    return llvm_builtin_binop(op_func, llvm_type, llvm_type, llvm_type,
			      lhs, rhs, module, builder);
  };
}

binop_table::op_codegen raytrace::llvm_sub_vec_vec(unsigned int N, type_table &types) {
  stringstream tname;
  tname << "vec" << N;
  Type *llvm_type = types[tname.str()]->llvm_type();

  return [N, llvm_type] (Value *lhs, Value *rhs,
	     Module *module, IRBuilder<> &builder) -> Value *{
    stringstream op_ss;
    op_ss << "gd_builtin_sub_v" << N << "_v" << N;
    string op_func = op_ss.str();

    return llvm_builtin_binop(op_func, llvm_type, llvm_type, llvm_type,
			      lhs, rhs, module, builder);
  };
}

binop_table::op_codegen raytrace::llvm_scale_vec(unsigned int N, bool swap_order, type_table &types) {
  stringstream tname;
  tname << "vec" << N;
  Type *llvm_type = types[tname.str()]->llvm_type();
  Type *f_type = types["float"]->llvm_type();
  
  return [N, swap_order, f_type, llvm_type] (Value *lhs, Value *rhs,
	     Module *module, IRBuilder<> &builder) -> Value *{
    stringstream op_ss;
    op_ss << "gd_builtin_scale_v" << N;
    string op_func = op_ss.str();

    Type *lhs_ty = f_type;
    Type *rhs_ty = llvm_type;

    if (swap_order) swap(lhs, rhs);
    return llvm_builtin_binop(op_func,
			      lhs_ty, rhs_ty, llvm_type,
			      lhs, rhs, module, builder);
  };
}

binop_table::op_codegen raytrace::llvm_inv_scale_vec(unsigned int N, type_table &types) {
  stringstream tname;
  tname << "vec" << N;
  Type *llvm_type = types[tname.str()]->llvm_type();
  Type *f_type = types["float"]->llvm_type();
  
  return [N, f_type, llvm_type] (Value *lhs, Value *rhs,
				 Module *module, IRBuilder<> &builder) -> Value *{
    stringstream op_ss;
    op_ss << "gd_builtin_inv_scale_v" << N;
    string op_func = op_ss.str();

    Type *lhs_ty = llvm_type;
    Type *rhs_ty = f_type;

    return llvm_builtin_binop(op_func,
			      lhs_ty, rhs_ty, llvm_type,
			      lhs, rhs, module, builder);
  };
}

binop_table::op_codegen raytrace::llvm_add_str_str(Type *str_type) {
  return [str_type] (Value *lhs, Value *rhs,
		     Module *module, IRBuilder<> &builder) -> Value* {
    Value *l_data = builder.CreateExtractValue(lhs, ArrayRef<unsigned int>(1), "s0_data");
    Value *r_data = builder.CreateExtractValue(rhs, ArrayRef<unsigned int>(1), "s1_data");
    
    Type *char_ptr = Type::getInt8PtrTy(getGlobalContext());
    vector<Type*> arg_ty{char_ptr, char_ptr};
    FunctionType *ft = FunctionType::get(char_ptr, ArrayRef<Type*>(arg_ty), false);
    Function *adder = GetExternalFunction(module, "gd_builtin_concat_string", ft);
    
    Value *new_data = builder.CreateCall(adder, vector<Value*>{l_data, r_data});
    Value *new_str = builder.CreateInsertValue(UndefValue::get(str_type),
					       ConstantInt::get(getGlobalContext(), APInt(1, false, true)), ArrayRef<unsigned int>(0), "str_concat_tmp");
    return builder.CreateInsertValue(new_str, new_data, ArrayRef<unsigned int>(1));
  };
}

binop_table::op_codegen raytrace::llvm_add_dfunc_dfunc(Type *dfunc_type) {
  return [dfunc_type] (Value *lhs, Value *rhs,
		       Module *module, IRBuilder<> &builder) -> Value* {
    Type *pointer_type = dfunc_type->getPointerTo();
    vector<Type*> arg_type({pointer_type, pointer_type, pointer_type});
    FunctionType *ty = FunctionType::get(Type::getVoidTy(getGlobalContext()), arg_type, false);
    Function *add_f = GetExternalFunction(module, "gd_builtin_dfunc_add", ty);
    
    Value *lhs_ptr = CreateEntryBlockAlloca(builder, dfunc_type, "dfunc_lhs");
    builder.CreateStore(lhs, lhs_ptr, false);

    Value *rhs_ptr = CreateEntryBlockAlloca(builder, dfunc_type, "dfunc_lhs");
    builder.CreateStore(rhs, rhs_ptr, false);

    Value *sum = CreateEntryBlockAlloca(builder, dfunc_type, "dfunc_add");
    builder.CreateCall3(add_f, lhs_ptr, rhs_ptr, sum);
    return builder.CreateLoad(sum);
  };
}

binop_table::op_codegen raytrace::llvm_scale_dfunc(Type *dfunc_type, bool swap_order, type_table &types) {
  return [dfunc_type, swap_order, &types] (Value *lhs, Value *rhs,
					   Module *module, IRBuilder<> &builder) -> Value* {
    if (swap_order) swap(lhs, rhs);
    Type *v4_type = types["vec4"]->llvm_type();
    Type *v4_ptr_type = v4_type->getPointerTo();
    Type *pointer_type = dfunc_type->getPointerTo();

    vector<Type*> arg_type({v4_ptr_type, pointer_type, pointer_type});
    FunctionType *ty = FunctionType::get(Type::getVoidTy(getGlobalContext()), arg_type, false);
    Function *add_f = GetExternalFunction(module, "gd_builtin_dfunc_scale", ty);
    
    Value *lhs_ptr = CreateEntryBlockAlloca(builder, v4_type, "dfunc_k");
    builder.CreateStore(lhs, lhs_ptr, false);
    
    Value *rhs_ptr = CreateEntryBlockAlloca(builder, dfunc_type, "dfunc_node");
    builder.CreateStore(rhs, rhs_ptr, false);

    Value *prod = CreateEntryBlockAlloca(builder, dfunc_type, "dfunc_scale");
    builder.CreateCall3(add_f, lhs_ptr, rhs_ptr, prod);
    return builder.CreateLoad(prod);
  };
}

binop_table::op_codegen raytrace::llvm_cmp_sf_sf() {
  return [] (Value *lhs, Value *rhs,
	     Module *, IRBuilder<> &builder) -> Value* {
    return builder.CreateICmpEQ(lhs, rhs);
  };
}

binop_table::op_codegen raytrace::llvm_add_sf_sf() {
  return [] (Value *lhs, Value *rhs,
	     Module *, IRBuilder<> &builder) -> Value* {
    return builder.CreateOr(lhs, rhs);
  };
}

binop_table::op_codegen raytrace::llvm_sub_sf_sf() {
  return [] (Value *lhs, Value *rhs,
	     Module *, IRBuilder<> &builder) -> Value* {
    Value *inv_rhs = builder.CreateNot(rhs);
    return builder.CreateAnd(lhs, inv_rhs);
  };
}

binop_table::op_codegen raytrace::llvm_and_sf_sf() {
  return [] (Value *lhs, Value *rhs,
	     Module *, IRBuilder<> &builder) -> Value* {
    Value *masked = builder.CreateAnd(lhs, rhs);
    return builder.CreateICmpNE(masked, ConstantInt::get(getGlobalContext(), APInt(64, 0, false)));
  };
}

/* Helpers */

Value *raytrace::llvm_builtin_binop(const string &func_name, 
				    Type *lhs_type, Type *rhs_type, Type *rt_type,
				    Value *lhs, Value *rhs,
				    Module *module, IRBuilder<> &builder) {
  Type *l_ptr_ty = PointerType::getUnqual(lhs_type);
  Type *r_ptr_ty = PointerType::getUnqual(rhs_type);
  Type *out_ptr_ty = PointerType::getUnqual(rt_type);
  
  vector<Type*> arg_ty{out_ptr_ty, l_ptr_ty, r_ptr_ty};
  FunctionType *ft = FunctionType::get(Type::getVoidTy(getGlobalContext()),
				       ArrayRef<Type*>(arg_ty), false);
  
  Function *op_func = GetExternalFunction(module, func_name.c_str(), ft);
  
  Value *l_ptr = CreateEntryBlockAlloca(builder, lhs_type, "tmp_lhs");
  builder.CreateStore(lhs, l_ptr, false);

  Value *r_ptr = CreateEntryBlockAlloca(builder, rhs_type, "tmp_rhs");
  builder.CreateStore(rhs, r_ptr, false);

  Value *out_ptr = CreateEntryBlockAlloca(builder, rt_type, "tmp_result");
    
  vector<Value*> args{out_ptr, l_ptr, r_ptr};
  builder.CreateCall(op_func, args);
  return builder.CreateLoad(out_ptr);
}

/** Unary Operations Table **/

unary_op_table::op_candidate_vector unary_op_table::find_operation(const string &op, const type_spec &type) const {
  op_candidate_vector candidates;

  auto table_it = operations.find(op);
  if (table_it != operations.end()) {
    auto op_it = table_it->second.find(type);
    if (op_it != table_it->second.end()) candidates.push_back(*op_it);
  }

  return candidates;
}

unary_op_table::op_candidate_value unary_op_table::find_best_operation(const std::string &op, const type_spec &type,
								       const type_conversion_table &conversions) const {
  auto table_it = operations.find(op);
  if (table_it == operations.end()) return errors::make_error<errors::error_message>(string("Unsupported operation: ") + op, 0, 0);
  
  const op_codegen_table &candidates = table_it->second;

  int max_score = std::numeric_limits<int>::max();
  int best_score = max_score;
  op_candidate_vector best_list;
  
  for (auto it = candidates.begin(); it != candidates.end(); ++it) {
    int score = candidate_score(it->first, type, conversions);
    if (score >= max_score) continue;
    
    if (score < best_score) {
      best_score = score;
      best_list.clear();
    }
    
    if (score == best_score) best_list.push_back(*it);
  }

  if (best_list.size() == 0) return errors::make_error<errors::error_message>("Invalid operation for this type", 0, 0); //no matches
  if (best_list.size() > 1) return errors::make_error<errors::error_message>("Operation is ambiguous", 0, 0); //ambiguous
  return best_list[0];
}

void unary_op_table::add_operation(const string &op, const type_spec &type,
				   const type_spec &result_type, const op_codegen &codegen) {
  op_info info{result_type, codegen};

  op_codegen_table &table = operations[op];
  table[type] = info;
}

int unary_op_table::candidate_score(const type_spec &expected_type,
				    const type_spec &type,
				    const type_conversion_table &conversions) const {
  int arg_cost, op_cost;
  
  if (!conversions.can_convert(type, expected_type, arg_cost, op_cost)) return std::numeric_limits<int>::max();
  return op_cost;
}

void unary_op_table::initialize_standard_ops(unary_op_table &table, type_table &types) {
  table.add_operation("!", types["bool"], types["bool"], llvm_not_b());
  table.add_operation("!", types["shader_flag"], types["shader_flag"], llvm_not_sf());

  table.add_operation("-", types["int"], types["int"], llvm_negate_i());
  table.add_operation("-", types["float"], types["float"], llvm_negate_f());
}

/* LLVM Unary Op Codegen Functions */

unary_op_table::op_codegen raytrace::llvm_not_b() {
  return [] (Value *arg,
	     Module *module, IRBuilder<> &builder) -> Value* {
    return builder.CreateNot(arg, "bool_not_tmp");
  };
}

unary_op_table::op_codegen raytrace::llvm_negate_i() {
  return [] (Value *arg,
	     Module *module, IRBuilder<> &builder) -> Value* {
    return builder.CreateNeg(arg, "i_neg_tmp");
  };
}

unary_op_table::op_codegen raytrace::llvm_negate_f() {
  return [] (Value *arg,
	     Module *module, IRBuilder<> &builder) -> Value* {
    return builder.CreateFNeg(arg, "f_neg_tmp");
  };
}

unary_op_table::op_codegen raytrace::llvm_not_sf() {
  return [] (Value *arg,
	     Module *module, IRBuilder<> &builder) -> Value* {
    return builder.CreateNot(arg, "sf_neg_tmp");
  };
}
