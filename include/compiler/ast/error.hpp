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

#ifndef GD_RL_AST_ERROR_HPP
#define GD_RL_AST_ERROR_HPP

#include "compiler/ast/statement.hpp"

namespace raytrace {

  namespace ast {

    /* Node that throws an error. */
    class error : public statement {
    public:

      error(parser_state *st,
	    const expression_ptr &error_str,
	    unsigned int line_no, unsigned int column_no);

      virtual codegen_void codegen(llvm::Module *module, llvm::IRBuilder<> &builder);

    private:

      expression_ptr error_str;

    };

  };

};

#endif
