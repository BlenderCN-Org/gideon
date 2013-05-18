#ifndef RT_RENDER_MODULE_HPP
#define RT_RENDER_MODULE_HPP

#include "compiler/ast/ast.hpp"

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/JIT.h"

namespace raytrace {

  /* A single source file in a renderer program. */
  struct render_object {
    
    render_object(const std::string &name, const std::string &source_code);
    
    //Parses the source code, throwing an exception if an error occurs.
    //Generates a syntax tree as well as a list of objects this object depends on.
    void parse(ast::parser_state *parser,
	       /* out */ std::vector<ast::global_declaration_ptr> &syntax_tree,
	       /* out */ std::vector<std::string> &object_dependencies);

    std::string name;
    std::string source_code;

    //Compiles the given syntax tree, throwing an exception if an error is encountered.
    static llvm::Module *compile(const std::string &name, ast::parser_state *parser,
				 std::vector<ast::global_declaration_ptr> &syntax_tree);
    
  };

  /* Compiled form of a render_program. */
  class compiled_renderer {
  public:

    compiled_renderer(llvm::Module *module);

    void *get_function_pointer(const std::string &func_name);
    void map_global(const std::string &name, void *location_ptr);
    
  private:
    
    llvm::Module *module;
    std::unique_ptr<llvm::ExecutionEngine> engine;

  };
  typedef compiled_renderer render_kernel;

  /* A group of objects that can be linked together to form a complete program. */
  class render_program {
  public:

    struct object_entry {
      render_object obj;
      ast::parser_state parser;
      std::vector<ast::global_declaration_ptr> syntax_tree;
      std::vector<std::string> dependencies;

      llvm::Module *module;

      object_entry(render_program *prog, const render_object &obj);
      void parse();
    };
    
    typedef boost::unordered_map<std::string, std::shared_ptr<object_entry>> object_table;
    
    render_program(const std::string &name);
    
    void add_object(const render_object &obj);
    void load_source_file(const std::string &path);

    //Compiles, links and optimizes all render objects in this program, returning the final result. */
    llvm::Module *compile();

    bool has_object(const std::string &name);
    export_table &get_export_table(const std::string &name);

  private:

    std::string name;
    object_table objects;

    type_table types;

    void optimize(llvm::Module *module);
    std::vector<std::string> generate_compile_order();
    void load_all_dependencies();
  };
  
  /* Defines set of functions that can be linked into a renderer program. */
  class render_module {
  public:

    render_module(const std::string &name, const std::string &source_code);
    ~render_module();

    //Returns a reference to a vector containing all libraries that this module depends on.
    const std::vector<std::string> &get_dependencies() const { return dependencies; }
    
    //Compile this render module into an LLVM module so that it may be linked into the final program.
    //The caller assumes ownership of the returned pointer.
    llvm::Module *compile();
    
  private:

    std::string name;
    std::string source;

    type_table types;
    ast::parser_state parser;
    std::vector<ast::global_declaration_ptr> top;
    std::vector<std::string> dependencies;

    //parses the source code to build the AST and dependencies.
    void parse_source();
    
    void optimize(llvm::Module *module);

  };
  
};

#endif
