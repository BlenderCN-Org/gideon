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
