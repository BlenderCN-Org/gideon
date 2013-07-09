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

#ifndef RT_AST_VARIABLE_HPP
#define RT_AST_VARIABLE_HPP

#include "compiler/types.hpp"
#include "compiler/ast/expression.hpp"
#include "compiler/ast/typename.hpp"
#include "compiler/ast/statement.hpp"
#include "compiler/ast/global.hpp"
#include "compiler/symboltable.hpp"

#include "compiler/errors.hpp"

namespace raytrace {
  
  namespace ast {
    
    /* Declares a new variable. */
    class variable_decl : public statement {
    public:

      variable_decl(parser_state *st, const std::string &name,
		    const type_expr_ptr &type, const expression_ptr &init,
		    unsigned int line_no, unsigned int column_no);
      virtual ~variable_decl() {}

      virtual codegen_void codegen(llvm::Module *module, llvm::IRBuilder<> &builder);
      
    private:
      
      std::string name;
      type_expr_ptr type;
      expression_ptr initializer;
      
      typed_value_container initialize_from_type(type_spec t, llvm::Module *module, llvm::IRBuilder<> &builder);
      codegen_void declare_with_type(type_spec t, llvm::Module *module, llvm::IRBuilder<> &builder);

    };
    
    /* Declares a new global variable. */
    
    class global_variable_decl : public global_declaration {
    public:
      
      global_variable_decl(parser_state *st,
			   const std::string &name, const type_expr_ptr &type,
			   const expression_ptr &init,
			   unsigned int line_no, unsigned int column_no);
      virtual ~global_variable_decl() { }

      virtual codegen_value codegen(llvm::Module *module, llvm::IRBuilder <> &builder);
      
    private:
      
      std::string name;
      type_expr_ptr type;
      expression_ptr initializer;

      std::string full_name();

    };

    /* A reference to a variable. */
    class variable_ref : public expression {
    public:

      variable_ref(parser_state *st, const std::string &name,
		   unsigned int line_no, unsigned int column_no);
      virtual ~variable_ref() {}
      
      virtual typecheck_value typecheck();
      virtual typed_value_container codegen(llvm::Module *module, llvm::IRBuilder<> &builder);
      virtual typed_value_container codegen_ptr(llvm::Module *module, llvm::IRBuilder<> &builder);
      
      virtual code_value codegen_module();

      virtual bool bound() const { return true; }
      
    private:
      
      std::string name;
      typed_value_container lookup_typed_var();
    };

    /* Type Constructors. */
    class type_constructor : public expression {
    public:

      type_constructor(parser_state *st, const type_expr_ptr &type, const std::vector<expression_ptr> &args,
		       unsigned int line_no, unsigned int column_no);
      ~type_constructor() {}

      virtual typecheck_value typecheck() { return type->codegen_type(); }
      virtual typed_value_container codegen(llvm::Module *module, llvm::IRBuilder<> &builder);
      virtual codegen_constant codegen_const_eval(llvm::Module *module, llvm::IRBuilder<> &builder);

    private:
      
      type_expr_ptr type;
      std::vector<expression_ptr> args;
      typed_value_container get_arg(int i, llvm::Module *module, llvm::IRBuilder<> &builder);
      
    };

    /* Access the field of a vector or struct (as the result of an expression). */
    class field_selection : public expression {
    public:

      field_selection(parser_state *st, const std::string &field, const expression_ptr &expr,
		      unsigned int line_no, unsigned int column_no);
      virtual ~field_selection() {}
      
      virtual typecheck_value typecheck();
      virtual typed_value_container codegen(llvm::Module *module, llvm::IRBuilder<> &builder);
      virtual typed_value_container codegen_ptr(llvm::Module *module, llvm::IRBuilder<> &builder);
      
      virtual code_value codegen_module();

      virtual bool bound() const { return expr->bound(); }
      
    private:

      std::string field;
      expression_ptr expr;
      
      typecheck_value typecheck_module_member(code_value &module_val);
      typed_value load_global(llvm::Value *ptr, type_spec &type,
			      llvm::Module *module, llvm::IRBuilder<> &builder);
    };

    /* Access an element in an array. */
    class element_selection : public expression {
    public:

      element_selection(parser_state *st, const expression_ptr &expr, const expression_ptr &idx_expr,
			unsigned int line_no, unsigned int column_no);
      virtual ~element_selection() {}
      
      virtual typecheck_value typecheck();
      virtual typed_value_container codegen(llvm::Module *module, llvm::IRBuilder<> &builder);
      virtual typed_value_container codegen_ptr(llvm::Module *module, llvm::IRBuilder<> &builder);
      
      virtual bool bound() const { return expr->bound(); }
      
    private:

      expression_ptr expr;
      expression_ptr idx_expr;
      
    };

  };

};

#endif
