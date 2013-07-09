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

#ifndef GD_RL_AST_TYPENAME_HPP
#define GD_RL_AST_TYPENAME_HPP

#include "compiler/types.hpp"
#include "compiler/ast/node.hpp"

#include <memory>

namespace raytrace {
  
  namespace ast {
    
    /* A node identifying a type. */
    class type_expression : public ast_node {
    public:
      
      type_expression(parser_state *st,
		      unsigned int line_no, unsigned int column_no) : ast_node(st, line_no, column_no) {}
      
      virtual typecheck_value codegen_type() = 0;
      
    };
    
    typedef std::shared_ptr<type_expression> type_expr_ptr;

    /* Identifies a type by name. */
    class typename_expression : public type_expression {
    public:
      
      typename_expression(parser_state *st,
			  const std::vector<std::string> &path, const std::string &name,
			  unsigned int line_no, unsigned int column_no) : type_expression(st, line_no, column_no),
									  path(path), name(name) {}
      typename_expression(parser_state *st,
			  const std::string &name,
			  unsigned int line_no, unsigned int column_no) : type_expression(st, line_no, column_no),
									  name(name) {}
      
      virtual typecheck_value codegen_type();
      
    private:
      
      std::vector<std::string> path;
      std::string name;
      
    };

    /* Array Type */
    class array_type_expression : public type_expression {
    public:

      array_type_expression(parser_state *st,
			    const type_expr_ptr &base, unsigned int N,
			    unsigned int line_no, unsigned int column_no) : type_expression(st, line_no, column_no),
									    base(base), N(N) {}
      
      virtual typecheck_value codegen_type();

    private:

      type_expr_ptr base;
      unsigned int N;

    };

    /* Array Reference */
    class array_ref_expression : public type_expression {
    public:
      
      array_ref_expression(parser_state *st,
			   const type_expr_ptr &base,
			   unsigned int line_no, unsigned int column_no) : type_expression(st, line_no, column_no),
									   base(base) {}
      
      virtual typecheck_value codegen_type();

    private:

      type_expr_ptr base;
      
    };
    

  };
  
};

#endif
