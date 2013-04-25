#ifndef RT_RENDER_MODULE_HPP
#define RT_RENDER_MODULE_HPP

#include "compiler/ast/ast.hpp"

namespace raytrace {

  /* Defines set of functions that can be linked into a renderer program. */
  class render_module {
  public:

    render_module(const std::string &name, const std::string &source_code);
    ~render_module();

    //Returns a reference to a vector containing all libraries that this module depends on.
    const std::vector<std::string> &get_dependencies() const { return dependencies; }
    
    struct distribution_export {
      std::string name;
      std::string eval_func_name;
    };

    //Returns a vector containing all the distributions defined by this block of code.
    std::vector<distribution_export> get_distributions() const;

    struct function_export {
      std::string name, full_name;
      type_spec return_type;
      std::vector<function_argument> arguments;
    };
    
    //Compile this render module into an LLVM module so that it may be linked into the final program.
    //The caller assumes ownership of the returned pointer.
    llvm::Module *compile();
    
  private:

    std::string name;
    std::string source;

    ast::parser_state parser;
    std::vector<ast::global_declaration_ptr> top;
    std::vector<std::string> dependencies;

    //parses the source code to build the AST and dependencies.
    void parse_source();

    //Request information about exported symbols from each global declaration.
    void load_exports();

    void optimize(llvm::Module *module);

  };

};

#endif
