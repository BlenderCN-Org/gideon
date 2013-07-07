#ifndef RT_AST_GLOBAL_HPP
#define RT_AST_GLOBAL_HPP

#include "compiler/ast/node.hpp"

namespace raytrace {

  namespace ast {
    
    /* Base class for declarations that may appear at the top-level of a shader file. */
    class global_declaration : public ast_node {
    public:

      global_declaration(parser_state *st) : ast_node(st) {}
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
