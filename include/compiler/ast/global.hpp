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

#ifndef RT_AST_GLOBAL_HPP
#define RT_AST_GLOBAL_HPP

#include "compiler/ast/node.hpp"

namespace raytrace {

  namespace ast {
    
    /* Base class for declarations that may appear at the top-level of a shader file. */
    class global_declaration : public ast_node {
    public:
      
      global_declaration(parser_state *st,
			 unsigned int line_no, unsigned int column_no) :
	ast_node(st, line_no, column_no) {}
      virtual ~global_declaration() {}
      virtual codegen_value codegen(llvm::Module *module, llvm::IRBuilder<> &builder) = 0;
    };

    typedef std::shared_ptr<global_declaration> global_declaration_ptr;
    
  };

};

#endif
