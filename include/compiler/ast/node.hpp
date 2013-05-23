#ifndef RT_AST_NODE_HPP
#define RT_AST_NODE_HPP

#include "compiler/errors.hpp"
#include "compiler/symboltable.hpp"
#include "compiler/gen_state.hpp"
#include "compiler/operations.hpp"
#include "compiler/lookup.hpp"

namespace raytrace {

  class render_program;

  namespace ast {
    
    /* Stores the state of the language parser. */
    struct parser_state {
      variable_symbol_table variables;
      function_symbol_table functions;
      module_symbol_table modules;
      
      export_table exports;
      type_table &types;
      binop_table binary_operations;
      unary_op_table unary_operations;
      control_state control;

      render_program *objects;
      boost::unordered_set<std::string> object_is_loaded;
      
      parser_state(type_table &t) : types(t) { }
    };
    
    class ast_node { 
    public:

      ast_node(parser_state *st,
	       unsigned int line_no = 0, unsigned int column_no = 0) : state(st), line_no(line_no), column_no(column_no) { }
      virtual ~ast_node() { }
      
    protected:

      parser_state *state;
      unsigned int line_no, column_no;

      void push_scope(const std::string &name = "");
      void pop_scope(llvm::Module *module, llvm::IRBuilder<> &builder);

      void push_function(const type_spec &t);
      void pop_function(llvm::Module *module, llvm::IRBuilder<> &builder);

      void push_module(const std::string &name);
      codegen_void pop_module(const std::string &name, llvm::Module *module, llvm::IRBuilder<> &builder);

      void push_distribution_context(const std::string &name, llvm::Type *param_ptr_type, const control_state::context_loader_type &loader);
      void pop_distribution_context(llvm::Module *module, llvm::IRBuilder<> &builder);

      void exit_loop_scopes(llvm::Module *module, llvm::IRBuilder<> &builder);
      void exit_to_loop_scope(llvm::Module *module, llvm::IRBuilder<> &builder);
      void exit_function(llvm::Module *module, llvm::IRBuilder<> &builder);
    
      variable_symbol_table &variables();

      function_symbol_table &functions();
      const function_symbol_table &functions() const;

      variable_scope &global_variables();
      function_scope &function_table();
      std::string function_scope_name();

      typecheck_value variable_type_lookup(const std::string &name);
      typed_value_container variable_lookup(const std::string &name);
    };

  };

};

#endif
