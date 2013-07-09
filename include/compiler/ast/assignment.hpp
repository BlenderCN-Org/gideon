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

#ifndef GD_RL_AST_ASSIGNMENT_HPP
#define GD_RL_AST_ASSIGNMENT_HPP

#include "compiler/ast/expression.hpp"

namespace raytrace {

  namespace ast {

    /* Assigns an expression to an lvalue. */
    class assignment : public expression {
    public:

      assignment(parser_state *st, const expression_ptr &lhs, const expression_ptr &rhs,
		 unsigned int line_no, unsigned int column_no);
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

      assignment_operator(parser_state *st, const std::string &op, bool return_prior,
			  const expression_ptr &lhs, const expression_ptr &rhs,
			  unsigned int line_no, unsigned int column_no);
      virtual ~assignment_operator() {}
      
      virtual typecheck_value typecheck();
      virtual typed_value_container codegen(llvm::Module *module, llvm::IRBuilder<> &builder);
      virtual typed_value_container codegen_ptr(llvm::Module *module, llvm::IRBuilder<> &builder);

      virtual bool bound() const { return !return_prior; }

    private:

      std::string op;
      expression_ptr lhs, rhs;
      bool return_prior;
      
      std::pair<typed_value_container, typed_value_container> get_value_and_pointer(llvm::Module *module, llvm::IRBuilder<> &builder);

      typed_value_container execute_assignment(binop_table::op_result_value &op_func,
					       llvm::Module *module, llvm::IRBuilder<> &builder,
					       type_spec &lhs_type, type_spec &rhs_type,
					       llvm::Value *lhs_ptr, llvm::Value *rhs_val);
      
    };

  };

};

#endif
