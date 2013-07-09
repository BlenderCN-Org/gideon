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

#ifndef RT_AST_CONTROL_HPP
#define RT_AST_CONTROL_HPP

#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include "compiler/types.hpp"
#include "compiler/gen_state.hpp"

#include "compiler/ast/statement.hpp"

namespace raytrace {
  
  namespace ast {

    /* if-else condition */
    class conditional_statement : public statement {
    public:

      conditional_statement(parser_state *st, const expression_ptr &cond,
			    const statement_ptr &if_branch, const statement_ptr &else_branch,
			    unsigned int line_no, unsigned int column_no);
      virtual ~conditional_statement() {}

      virtual codegen_void codegen(llvm::Module *module, llvm::IRBuilder<> &builder);

    private:

      expression_ptr cond;
      statement_ptr if_branch, else_branch;

    };

    /* for-loop construct */
    class for_loop_statement : public statement {
    public:

      for_loop_statement(parser_state *st,
			 const statement_ptr &init,
			 const expression_ptr &cond,
			 const expression_ptr &after,
			 const statement_ptr &body,
			 unsigned int line_no, unsigned int column_no);

      virtual ~for_loop_statement() {}

      virtual codegen_void codegen(llvm::Module *module, llvm::IRBuilder<> &builder);

    private:

      statement_ptr init, body;
      expression_ptr cond, after;
      
    };

    /* Break Statement */
    class break_statement : public statement {
    public:

      break_statement(parser_state *st,
		      unsigned int line_no, unsigned int column_no);
      virtual ~break_statement() {}

      virtual codegen_void codegen(llvm::Module *module, llvm::IRBuilder<> &builder);
      virtual bool is_terminator() { return true; }

    };

   /* Continue Statement */
    class continue_statement : public statement {
    public:

      continue_statement(parser_state *st,
			 unsigned int line_no, unsigned int column_no);
      virtual ~continue_statement() {}

      virtual codegen_void codegen(llvm::Module *module, llvm::IRBuilder<> &builder);
      virtual bool is_terminator() { return true; }
      
    };

  };

};

#endif
