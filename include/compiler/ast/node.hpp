#ifndef RT_AST_NODE_HPP
#define RT_AST_NODE_HPP

#include "compiler/errors.hpp"
#include "compiler/symboltable.hpp"
#include "compiler/gen_state.hpp"

namespace raytrace {

  namespace ast {
    
    /* Stores the state of the language parser. */
    struct parser_state {
      variable_symbol_table variables;
      function_symbol_table functions;
      
      type_table types;
      control_state control;
    };
    
    class ast_node { 
    public:

      ast_node(parser_state *st,
	       unsigned int line_no = 0, unsigned int column_no = 0) : state(st), line_no(line_no), column_no(column_no) { }
      virtual ~ast_node() { }
      
    protected:

      parser_state *state;
      unsigned int line_no, column_no;

    };

  };

};

#endif
