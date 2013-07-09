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

#ifndef RT_AST_STATEMENT_HPP
#define RT_AST_STATEMENT_HPP

#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Analysis/Verifier.h"

#include "compiler/symboltable.hpp"
#include "compiler/ast/expression.hpp"

#include <memory>
#include <vector>

namespace raytrace {

  namespace ast {

    /* Generic statement node. */
    class statement : public ast_node {
    public:

      statement(parser_state *st, unsigned int line_no, unsigned int column_no) : ast_node(st, line_no, column_no) {}
      virtual ~statement() {}

      virtual codegen_void codegen(llvm::Module *module, llvm::IRBuilder<> &builder) = 0;
      virtual bool is_terminator() { return false; }
    };

    typedef std::shared_ptr<statement> statement_ptr;

    /* Statement containing an expression. */
    class expression_statement : public statement {
    public:

      expression_statement(parser_state *st, const expression_ptr &expr,
			   unsigned int line_no, unsigned int column_no);
      virtual ~expression_statement() {}

      virtual codegen_void codegen(llvm::Module *module, llvm::IRBuilder<> &builder);
      
    private:
      
      expression_ptr expr;

    };

    /* Lists of statements. */
    class statement_list {
    public:

      statement_list(const std::vector<statement_ptr> &statements);
      codegen_void codegen(llvm::Module *module, llvm::IRBuilder<> &builder);

    private:

      std::vector<statement_ptr> statements;

    };

    /* Statements executing within a given scope. */
    class scoped_statement : public statement {
    public:

      scoped_statement(parser_state *st, const statement_list &stmt_list,
		       unsigned int line_no, unsigned int column_no);
      virtual ~scoped_statement() { }

      virtual codegen_void codegen(llvm::Module *module, llvm::IRBuilder<> &builder);

    private:
      
      statement_list statements;
      
    };

  };

};

#endif
