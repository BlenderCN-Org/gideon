#ifndef RT_AST_FUNCTION_HPP
#define RT_AST_FUNCTION_HPP

#include "compiler/ast/statement.hpp"
#include "compiler/ast/expression.hpp"
#include "compiler/ast/typename.hpp"
#include "compiler/ast/global.hpp"
#include "compiler/symboltable.hpp"
#include "compiler/gen_state.hpp"

#include <vector>

namespace raytrace {
  
  namespace ast {

    /* Expression that calls a particular function and returns the result. */
    class func_call : public expression {
    public:

      func_call(parser_state *st,
		const expression_ptr &path_expr, const std::string &fname,
		const std::vector<expression_ptr> &args,
		unsigned int line_no, unsigned int column_no);
      virtual ~func_call() {}

      virtual typecheck_value typecheck();
      virtual typed_value_container codegen(llvm::Module *module, llvm::IRBuilder<> &builder);
      
    private:

      expression_ptr path_expr;
      std::string fname;
      std::vector<expression_ptr> args;
      
      typed_value_vector codegen_all_args(entry_or_error &entry,
					  llvm::Module *module, llvm::IRBuilder<> &builder,
					  /* out */ std::vector<typed_value_container> &to_destroy);
      ast_node::entry_or_error lookup_function();
    };

    /* A type expression used in a function parameter. */
    struct function_parameter {
      std::string name;
      type_expr_ptr type;
      bool output;
    };

    /* 
       Represents a function prototype, defining the return type, name and arguments of a function.
       A function may be declared multiple times, as long as the body is only defined once and all
       prototypes match.
     */
    class prototype : public global_declaration {
    public:
      
      //Locally defined functions.
      prototype(parser_state *st, const std::string &name, const type_expr_ptr &return_type,
		const std::vector<function_parameter> &args);

      //Externally defined functions.
      prototype(parser_state *st, const std::string &name, const std::string &extern_name,
		const type_expr_ptr &return_type, const std::vector<function_parameter> &args);
      
      virtual ~prototype() {}

      virtual codegen_value codegen(llvm::Module *module, llvm::IRBuilder<> &builder);
      
      const std::string &function_name() { return name; }
      void set_external(const std::string &extern_name);
      
      typedef raytrace::codegen<function_symbol_table::entry_type, compile_error>::value function_gen_value;
      function_gen_value codegen_entry(llvm::Module *module, llvm::IRBuilder<> &builder);
      
    private:

      std::string name, extern_name;
      type_expr_ptr return_type;
      std::vector<function_parameter> args;
      bool external, member_function;
      
      //checks to see if this function has been previously defined (and if so, do the prototypes match).
      entry_or_error check_for_entry(const std::vector<type_spec> &arg_types, const type_spec &return_ty);
      
      function_key get_key(const std::vector<type_spec> &arg_types) const;
      
      typecheck_vector get_arg_types();
    };

    typedef std::shared_ptr<prototype> prototype_ptr;
    
    /* Represents a full function definition. */
    class function : public global_declaration {
    public:
      
      function(parser_state *st, const prototype_ptr &defn, const statement_list &body);
      virtual ~function() {}

      virtual codegen_value codegen(llvm::Module *module, llvm::IRBuilder<> &builder);
      
    private:

      prototype_ptr defn;
      statement_list body;
      
      codegen_value create_function(function_entry &entry, llvm::Module *module, llvm::IRBuilder<> &builder);

    };

    typedef std::shared_ptr<function> function_ptr;

    /* Returns a value from a function. The expression pointer may be NULL, in which case nothing is returned. */
    class return_statement : public statement {
    public:
      
      return_statement(parser_state *st, const expression_ptr &expr);
      virtual ~return_statement() {}

      virtual codegen_void codegen(llvm::Module *module, llvm::IRBuilder<> &builder);
      virtual bool is_terminator() { return true; }

    private:
      
      expression_ptr expr;

    };
    
  };
  
};

#endif
