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

#ifndef RT_AST_LITERAL_HPP
#define RT_AST_LITERAL_HPP

#include "compiler/ast/expression.hpp"

namespace raytrace {

  namespace ast {
    
    template<typename T>
    type_spec get_literal_type(parser_state *st);

    //Template class for literals.
    template<typename T>
    class literal : public expression {
    public:
      
      literal(parser_state *st, const T& v,
	      unsigned int line_no, unsigned int column_no) : expression(st, get_literal_type<T>(st),
									 line_no, column_no), value(v) {}
      virtual ~literal() { }
      
      virtual typed_value_container codegen(llvm::Module *module, llvm::IRBuilder<> &builder);
      virtual typecheck_value typecheck() { return { get_literal_type<T>(state) }; }
      virtual codegen_constant codegen_const_eval(llvm::Module *module, llvm::IRBuilder<> &builder);
      
    private:
      
      T value;
      
    };

  };

};

#endif
