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

#ifndef GD_RL_AST_ALIAS_HPP
#define GD_RL_AST_ALIAS_HPP

#include "compiler/ast/global.hpp"

namespace raytrace {

  namespace ast {

    /* Generates an alias of a variable name. */
    class global_variable_alias : public global_declaration {
    public:

      global_variable_alias(parser_state *st,
			    const std::string &full_name, const type_spec &type,
			    const std::string &alias_name,
			    unsigned int line_no, unsigned int column_no);
      virtual ~global_variable_alias() { }

      virtual codegen_value codegen(llvm::Module *module, llvm::IRBuilder<> &builder);

    private:

      std::string alias_name, full_name;
      type_spec type;
      
    };

    /* Generates an alias of a function name. */
    class function_alias : public global_declaration {
    public:

      //the 'func' field of entry may be left as NULL. it will be ignored
      function_alias(parser_state *st,
		     const function_entry &entry,
		     const std::string &alias_name,
		     unsigned int line_no, unsigned int column_no);

      virtual ~function_alias() { }

      virtual codegen_value codegen(llvm::Module *module, llvm::IRBuilder<> &builder);

    private:

      function_entry func;
      std::string alias_name;

    };
    
  };

};

#endif
