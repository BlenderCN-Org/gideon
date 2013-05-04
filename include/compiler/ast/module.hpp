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
	     unsigned int line_no, unsigned int column_no) :
	global_declaration(st),
	name(name), content(content)
      { }
      virtual ~module() { }

      //Evaluates all global declarations contained within this module.
      codegen_value codegen(llvm::Module *module, llvm::IRBuilder<> &builder);
      
    private:

      std::string name;
      std::vector<global_declaration_ptr> content;

    };

  };

};

#endif
