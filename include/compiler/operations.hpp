#ifndef RT_OPERATIONS_HPP
#define RT_OPERATIONS_HPP

#include "compiler/types.hpp"
#include "llvm/IRBuilder.h"

namespace raytrace {

  /* A table containing all valid binary operations. */
  class binop_table {
  public:
    
    typedef std::pair<type_spec, type_spec> op_type;    
    typedef boost::function<llvm::Value *(llvm::Value *, llvm::Value *,
					  llvm::Module *, llvm::IRBuilder<> &)> op_codegen;
    
    struct op_info {
      type_spec result_type;
      op_codegen codegen;
    };

    typedef std::pair<op_type, op_info> op_result;
    typedef std::vector<op_result> op_candidate_vector;
    typedef codegen<op_result, compile_error>::value op_result_value;

    //Given an operation and the operand types, returns a list of potential operations.
    op_candidate_vector find_operation(const std::string &op, const type_spec &lhs, const type_spec &rhs) const;

    //Accounting for cast operations, finds the most appropriate version of the given operation to use.
    op_result_value find_best_operation(const std::string &op, const type_spec &lhs, const type_spec &rhs) const;
    
    //Inserts a new operation into the table.
    void add_operation(const std::string &op, const type_spec &lhs, const type_spec &rhs,
		       const type_spec &result_type, const op_codegen &codegen);

    static void initialize_standard_ops(binop_table &table, type_table &types);

  private:

    typedef boost::unordered_map<op_type, op_info> op_codegen_table;    
    boost::unordered_map<std::string, op_codegen_table> operations;

    int candidate_score(const op_type &types,
			const type_spec &lhs, const type_spec &rhs) const;
    
  };
 
  /* LLVM Operations */

  binop_table::op_codegen llvm_add_i_i();
  binop_table::op_codegen llvm_sub_i_i();
  binop_table::op_codegen llvm_mul_i_i();
  binop_table::op_codegen llvm_lt_i_i();

  binop_table::op_codegen llvm_add_f_f();
  binop_table::op_codegen llvm_sub_f_f();
  binop_table::op_codegen llvm_mul_f_f();
  binop_table::op_codegen llvm_div_f_f();

  binop_table::op_codegen llvm_lt_f_f();
  binop_table::op_codegen llvm_gt_f_f();

  binop_table::op_codegen llvm_add_vec_vec(unsigned int N, type_table &types);
  binop_table::op_codegen llvm_mul_vec_vec(unsigned int N, type_table &types);
  binop_table::op_codegen llvm_sub_vec_vec(unsigned int N, type_table &types);
  binop_table::op_codegen llvm_scale_vec(unsigned int N, bool swap_order, type_table &types);
  
  binop_table::op_codegen llvm_add_str_str(llvm::Type *str_type);
 
  binop_table::op_codegen llvm_add_dfunc_dfunc(llvm::Type *dfunc_type);

  /* Unary Operations */

  class unary_op_table {
  public:
  
    typedef boost::function<llvm::Value *(llvm::Value *,
					  llvm::Module *, llvm::IRBuilder<> &)> op_codegen;

    struct op_info {
      type_spec result_type;
      op_codegen codegen;
    };

    typedef std::pair<type_spec, op_info> op_candidate;
    typedef std::vector<op_candidate> op_candidate_vector;
    typedef codegen<op_candidate, compile_error>::value op_candidate_value;

    //Given an operation and the operand type, returns a list of potential operations.
    op_candidate_vector find_operation(const std::string &op, const type_spec &type) const;

    //Accounting for cast operations, finds the most appropriate version of the given operation to use.
    op_candidate_value find_best_operation(const std::string &op, const type_spec &type) const;
    
    //Inserts a new operation into the table.
    void add_operation(const std::string &op, const type_spec &ty,
		       const type_spec &result_type, const op_codegen &codegen);

    static void initialize_standard_ops(unary_op_table &table, type_table &types);

  private:

    typedef boost::unordered_map<type_spec, op_info> op_codegen_table;    
    boost::unordered_map<std::string, op_codegen_table> operations;

    int candidate_score(const type_spec &expected_type,
			const type_spec &type) const;
    
  };

  /* LLVM Unary Operations */

  unary_op_table::op_codegen llvm_not_b();

  unary_op_table::op_codegen llvm_negate_i();
  
  unary_op_table::op_codegen llvm_negate_f();

  //Helpers

  llvm::Value *llvm_builtin_binop(const std::string &func_name,
				  llvm::Type *lhs_type, llvm::Type *rhs_type, llvm::Type *rt_type,
				  llvm::Value *lhs, llvm::Value *rhs,
				  llvm::Module *module, llvm::IRBuilder<> &builder);
};

#endif
