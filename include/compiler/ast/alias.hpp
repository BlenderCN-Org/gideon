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
