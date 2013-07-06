#ifndef RT_RENDER_MODULE_HPP
#define RT_RENDER_MODULE_HPP

#include "compiler/ast/ast.hpp"

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/JIT.h"

namespace raytrace {

  /* Helper functions for loading source files. */
  std::string basic_path_resolver(const std::string &src_path_str,
				  const std::vector<std::string> &search_paths);
  std::string basic_source_loader(const std::string &abs_path);

  /* A single source file in a renderer program. */
  struct render_object {
    
    render_object(const std::string &name, const std::string &source_code);
    
    //Parses the source code, throwing an exception if an error occurs.
    //Generates a syntax tree as well as a list of objects this object depends on.
    codegen_void parse(ast::parser_state *parser,
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
      codegen_void parse();
    };
    
    typedef boost::unordered_map<std::string, std::shared_ptr<object_entry>> object_table;
    
    render_program(const std::string &name);
    render_program(const std::string &name,
		   const boost::function<std::string (const std::string &)> &resolve,
		   const boost::function<std::string (const std::string &)> &loader);
    
    void add_object(const render_object &obj);
    void load_source_file(const std::string &obj_name);
    std::string get_object_name(const std::string &path);

    //Calls the given function on any exported function matching the given type.
    //May only be called after the program's been compiled.
    void foreach_function_type(exports::function_export::export_type type,
			       const boost::function<void (const std::string &, const std::string &)> &on_function) const;

    //Compiles, links and optimizes all render objects in this program, returning the final result.
    llvm::Module *compile();

    bool has_object(const std::string &name);
    export_table &get_export_table(const std::string &name);

  private:

    std::string name;
    object_table objects;

    boost::function <std::string (const std::string &)> path_resolver, source_loader;
    std::string default_loader(const std::string &path);
    
    type_table types;

    void optimize(llvm::Module *module);
    std::vector<std::string> generate_compile_order();
    void load_all_dependencies();

    void load_resolved_filename(const std::string &path);
  };
  
};

#endif
