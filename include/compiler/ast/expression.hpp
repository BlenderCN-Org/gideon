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

#ifndef RT_AST_EXPRESSION_HPP
#define RT_AST_EXPRESSION_HPP

#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Analysis/Verifier.h"

#include "compiler/types.hpp"
#include "compiler/ast/node.hpp"

#include <memory>
#include <stdexcept>

namespace raytrace {

  namespace ast {
    
    /* Base class for expressions */
    class expression : public ast_node {
    public:

      expression(parser_state *st, unsigned int line_no, unsigned int column_no) : ast_node(st, line_no, column_no), result_type(st->types["void"]) {}
      expression(parser_state *st, const type_spec &rt,
		 unsigned int line_no, unsigned int column_no) : ast_node(st, line_no, column_no), result_type(rt) {}
      
      virtual ~expression() {}

      virtual typecheck_value typecheck() = 0;

      virtual typed_value_container codegen(llvm::Module *module, llvm::IRBuilder<> &builder) = 0;
      virtual typed_value_container codegen_ptr(llvm::Module *module, llvm::IRBuilder<> &builder);

      //returns true if the result of this expression is bound to a variable,
      //which basically means it has an address.
      //if this function returns true, then codegen_ptr() must also be implemented
      virtual bool bound() const { return false; }
      static void destroy_unbound(typed_value_container &val, llvm::Module *module, llvm::IRBuilder<> &builder);
      
      //special function for use in type checking, where we may need the module's name, but can't call codegen() directly.
      virtual code_value codegen_module();

      //if this is a constant expression, evaluate it
      virtual codegen_constant codegen_const_eval(llvm::Module *module, llvm::IRBuilder<> &builder);

    protected:

      type_spec result_type;
      
    };

    typedef std::shared_ptr<expression> expression_ptr;
    
    /* A binary operation */
    class binary_expression : public expression {
    public:
      
      binary_expression(parser_state *st, const std::string &op, const expression_ptr &lhs, const expression_ptr &rhs,
			unsigned int line_no, unsigned int column_no);
      virtual ~binary_expression() {}
      
      virtual typed_value_container codegen(llvm::Module *module, llvm::IRBuilder<> &builder);
      virtual typecheck_value typecheck();

    private:
      
      std::string op;
      expression_ptr lhs, rhs;
      
      typed_value_container execute_op(binop_table::op_result_value &op_func,
				       llvm::Module *module, llvm::IRBuilder<> &builder,
				       llvm::Value* lhs_val, llvm::Value *rhs_val,
				       const type_spec &lhs_type, const type_spec &rhs_type);
    };

    /* A unary arithmetic operation */
    class unary_op_expression : public expression {
    public:
      
      unary_op_expression(parser_state *st, const std::string &op, const expression_ptr &arg,
			  unsigned int line_no, unsigned int column_no);
      virtual ~unary_op_expression() {}

      virtual typed_value_container codegen(llvm::Module *module, llvm::IRBuilder<> &builder);
      virtual typecheck_value typecheck();

    private:
      
      std::string op;
      expression_ptr arg;
      
      typed_value_container execute_op(unary_op_table::op_candidate_value &op_func,
				       llvm::Module *module, llvm::IRBuilder<> &builder,
				       llvm::Value* arg_val, type_spec &arg_type);
    };
  
  };
};

#endif
