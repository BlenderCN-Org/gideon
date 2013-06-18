#ifndef RT_AST_NODE_HPP
#define RT_AST_NODE_HPP

#include "compiler/errors.hpp"
#include "compiler/symboltable.hpp"
#include "compiler/gen_state.hpp"
#include "compiler/type_conversion.hpp"
#include "compiler/operations.hpp"
#include "compiler/lookup.hpp"

namespace raytrace {

  class render_program;

  namespace ast {
    
    /* Stores the state of the language parser. */
    struct parser_state {
      type_table &types;
      
      type_conversion_table type_conversions;
      binop_table binary_operations;
      unary_op_table unary_operations;

      variable_symbol_table variables;
      function_symbol_table functions;
      type_symbol_table user_types;
      module_symbol_table modules;
      
      export_table exports;
      
      control_state control;

      render_program *objects;
      boost::unordered_set<std::string> object_is_loaded;
      
      parser_state(type_table &t) : types(t), type_conversions(t) { }
    };
    
    class ast_node { 
    public:

      ast_node(parser_state *st,
	       unsigned int line_no = 0, unsigned int column_no = 0) : state(st), line_no(line_no), column_no(column_no) { }
      virtual ~ast_node() { }

      typedef raytrace::codegen<function_symbol_table::entry_type*, compile_error>::value entry_or_error;
      
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
      
      code_value typecast(typed_value_container &src, const type_spec &dst_type,
			  bool make_copy, bool destroy_original,
			  llvm::Module *module, llvm::IRBuilder<> &builder);
      
      code_value typecast(llvm::Value *src, 
			  const type_spec &src_type, const type_spec &dst_type,
			  bool make_copy, bool destroy_on_convert,
			  llvm::Module *module, llvm::IRBuilder<> &builder);

      //Lookup Functions

      typecheck_value variable_type_lookup(const std::string &name);
      typed_value_container variable_lookup(const std::string &name);

      entry_or_error function_lookup(const function_key &fkey);

      typecheck_value typename_lookup(const std::string &name);
    };

  };

};

#endif
