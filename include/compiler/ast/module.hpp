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

#ifndef GD_RL_AST_MODULE_HPP
#define GD_RL_AST_MODULE_HPP

#include "compiler/ast/node.hpp"
#include "compiler/ast/global.hpp"

namespace raytrace {

  namespace ast {

    /* Defines a module containing a set of functions, distributions and shaders. */
    class module : public global_declaration {
    public:

      module(parser_state *st,
	     const std::string &name, const std::vector<global_declaration_ptr> &content,
	     bool exported,
	     unsigned int line_no, unsigned int column_no) :
	global_declaration(st, line_no, column_no),
	name(name), content(content), exported(exported)
      { }
      virtual ~module() { }

      //Evaluates all global declarations contained within this module.
      virtual codegen_value codegen(llvm::Module *module, llvm::IRBuilder<> &builder);
      
    private:

      std::string name;
      std::vector<global_declaration_ptr> content;
      bool exported;

    };

    /* Imports another module's symbols into the current module. */
    class expression;
    class import_declaration : public global_declaration {
    public:

      typedef std::shared_ptr<expression> expression_ptr;

      import_declaration(parser_state *st, const expression_ptr &module_path,
			 unsigned int line_no, unsigned int column_no);
      virtual ~import_declaration() { }

      virtual codegen_value codegen(llvm::Module *module, llvm::IRBuilder<> &builder);

    private:
      
      expression_ptr module_path;

      void export_module(const std::string &name, module_ptr &m);
      
    };

    /* Loads exports from an export table into this current syntax tree. */
    class load_declaration : public global_declaration {
    public:

      load_declaration(parser_state *st, const std::string &source_name,
		       unsigned int line_no, unsigned int column_no);
      virtual ~load_declaration() { }

      virtual codegen_value codegen(llvm::Module *module, llvm::IRBuilder<> &builder);

    private:

      std::string source_name;

      bool has_export_table();
      export_table &get_export_table();
      std::vector<global_declaration_ptr> generate_subtree(export_table &exports);
      global_declaration_ptr get_module_content(exports::module_export::pointer &m);

      bool is_loaded();
      void set_loaded();
      
    };

  };

};

#endif
