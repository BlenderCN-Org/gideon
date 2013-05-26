#ifndef GD_RL_AST_ASSIGNMENT_HPP
#define GD_RL_AST_ASSIGNMENT_HPP

#include "compiler/ast/expression.hpp"

namespace raytrace {

  namespace ast {

    /* Assigns an expression to an lvalue. */
    class assignment : public expression {
    public:

      assignment(parser_state *st, const expression_ptr &lhs, const expression_ptr &rhs);
      virtual ~assignment() {}
      
      virtual typecheck_value typecheck();
      virtual typed_value_container codegen(llvm::Module *module, llvm::IRBuilder<> &builder);
      virtual typed_value_container codegen_ptr(llvm::Module *module, llvm::IRBuilder<> &builder);

      virtual bool bound() const { return true; }

    private:

      expression_ptr lhs, rhs;

      std::pair<typed_value_container, typed_value_container> get_value_and_pointer(llvm::Module *module, llvm::IRBuilder<> &builder);
      
    };

    /* Performs a binary operation, assigning the result to the given lvalue. */
    class assignment_operator : public expression {
    public:

      assignment_operator(parser_state *st, const std::string &op,
			  const expression_ptr &lhs, const expression_ptr &rhs,
			  unsigned int line_no, unsigned int column_no);
      virtual ~assignment_operator() {}
      
      virtual typecheck_value typecheck();
      virtual typed_value_container codegen(llvm::Module *module, llvm::IRBuilder<> &builder);
      virtual typed_value_container codegen_ptr(llvm::Module *module, llvm::IRBuilder<> &builder);

      virtual bool bound() const { return true; }

    private:

      std::string op;
      expression_ptr lhs, rhs;
      
      std::pair<typed_value_container, typed_value_container> get_value_and_pointer(llvm::Module *module, llvm::IRBuilder<> &builder);

      typed_value_container execute_assignment(binop_table::op_result_value &op_func,
					       llvm::Module *module, llvm::IRBuilder<> &builder,
					       type_spec &lhs_type,
					       llvm::Value *lhs_ptr, llvm::Value *rhs_val);
      
    };

  };

};

#endif
