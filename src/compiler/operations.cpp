#include "compiler/operations.hpp"
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

binop_table::op_result_value binop_table::find_best_operation(const std::string &op, const type_spec &lhs, const type_spec &rhs) const {
  auto table_it = operations.find(op);
  if (table_it == operations.end()) return compile_error(string("Unsupported operation: ") + op);
  
  const op_codegen_table &candidates = table_it->second;

  int max_score = std::numeric_limits<int>::max();
  int best_score = max_score;
  op_candidate_vector best_list;

  for (auto it = candidates.begin(); it != candidates.end(); ++it) {
    int score = candidate_score(it->first, lhs, rhs);
    if (score >= max_score) continue;
    
    if (score < best_score) {
      best_score = score;
      best_list.clear();
    }
    
    if (score == best_score) best_list.push_back(*it);
  }

  if (best_list.size() == 0) return compile_error("Invalid operation for those types"); //no matches
  if (best_list.size() > 1) return compile_error("Binary operation is ambiguous"); //ambiguous
  return best_list[0];
}

void binop_table::add_operation(const string &op, const type_spec &lhs, const type_spec &rhs,
				const type_spec &result_type, const op_codegen &codegen) {
  op_type args = make_pair(lhs, rhs);
  op_info info{result_type, codegen};

  op_codegen_table &table = operations[op];
  table[args] = info;
}

int binop_table::candidate_score(const op_type &types,
				 const type_spec &lhs, const type_spec &rhs) const {
  int l_cost, r_cost;
  if (!lhs->can_cast_to(*types.first, l_cost)) return l_cost;
  if (!rhs->can_cast_to(*types.second, r_cost)) return r_cost;

  return l_cost + r_cost;
}

void binop_table::initialize_standard_ops(binop_table &table, type_table &types) {
  table.add_operation("+", types["int"], types["int"], types["int"], llvm_add_i_i());
  table.add_operation("<", types["int"], types["int"], types["bool"], llvm_lt_i_i());

  table.add_operation("+", types["float"], types["float"], types["float"], llvm_add_f_f());

  table.add_operation("+", types["vec2"], types["vec2"], types["vec2"], llvm_add_vec_vec(2, types));
  table.add_operation("+", types["vec3"], types["vec3"], types["vec3"], llvm_add_vec_vec(3, types));
  table.add_operation("+", types["vec4"], types["vec4"], types["vec4"], llvm_add_vec_vec(4, types));

  table.add_operation("+", types["string"], types["string"], types["string"],
  		      llvm_add_str_str(types["string"]->llvm_type()));
}

/* LLVM Code Generation Functions */

binop_table::op_codegen raytrace::llvm_add_i_i() {
  return [] (Value *lhs, Value *rhs,
	     Module *module, IRBuilder<> &builder) -> Value *{
    return builder.CreateAdd(lhs, rhs, "i_add_tmp");
  };
}

binop_table::op_codegen raytrace::llvm_lt_i_i() {
  return [] (Value *lhs, Value *rhs,
	     Module *module, IRBuilder<> &builder) -> Value *{
    return builder.CreateICmpSLT(lhs, rhs, "i_lt_tmp");
  };
}

binop_table::op_codegen raytrace::llvm_add_f_f() {
  return [] (Value *lhs, Value *rhs,
	     Module *module, IRBuilder<> &builder) -> Value *{
    return builder.CreateFAdd(lhs, rhs, "f_add_tmp");
  };
}

binop_table::op_codegen raytrace::llvm_add_vec_vec(unsigned int N, type_table &types) {
  stringstream tname;
  tname << "vec" << N;
  Type *llvm_type = types[tname.str()]->llvm_type();

  return [N, llvm_type] (Value *lhs, Value *rhs,
	     Module *module, IRBuilder<> &builder) -> Value *{
    stringstream op_ss;
    op_ss << "gd_builtin_sub_v" << N << "_v" << N;
    string op_func = op_ss.str();

    return llvm_builtin_binop(op_func, llvm_type, lhs, rhs, module, builder);
    
    return builder.CreateFAdd(lhs, rhs, "f_add_tmp");
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
    Function *adder = cast<Function>(module->getOrInsertFunction("gd_builtin_concat_string", ft));
    
    Value *new_data = builder.CreateCall(adder, vector<Value*>{l_data, r_data});
    Value *new_str = builder.CreateInsertValue(UndefValue::get(str_type),
					       ConstantInt::get(getGlobalContext(), APInt(1, false, true)), ArrayRef<unsigned int>(0), "str_concat_tmp");
    return builder.CreateInsertValue(new_str, new_data, ArrayRef<unsigned int>(1));
  };
}

/* Helpers */

Value *raytrace::llvm_builtin_binop(const string &func_name, Type *type, Value *lhs, Value *rhs,
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
