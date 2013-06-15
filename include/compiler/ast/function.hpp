#ifndef RT_AST_FUNCTION_HPP
#define RT_AST_FUNCTION_HPP

#include "compiler/ast/statement.hpp"
#include "compiler/ast/expression.hpp"
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

    /* 
       Represents a function prototype, defining the return type, name and arguments of a function.
       A function may be declared multiple times, as long as the body is only defined once and all
       prototypes match.
     */
    class prototype : public global_declaration {
    public:
      
      //Locally defined functions.
      prototype(parser_state *st, const std::string &name, const type_spec &return_type,
		const std::vector<function_argument> &args);

      //Externally defined functions.
      prototype(parser_state *st, const std::string &name, const std::string &extern_name,
		const type_spec &return_type, const std::vector<function_argument> &args);
      
      virtual ~prototype() {}

      virtual codegen_value codegen(llvm::Module *module, llvm::IRBuilder<> &builder);
      
      const std::string &function_name() { return name; }
      void set_external(const std::string &extern_name);
      
      size_t num_args() { return args.size(); }
      const function_argument &get_arg(int i) { return args[i]; }
      const type_spec &get_return_type() { return return_type; }

      bool is_external() { return external; }
      bool is_member_function() { return member_function; } //this will only be known after codegen
            
    private:

      std::string name, extern_name;
      type_spec return_type;
      std::vector<function_argument> args;
      bool external, member_function;
      
      //checks to see if this function has been previously defined (and if so, do the prototypes match).
      codegen_value check_for_entry();

      function_key get_key() const;
      
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
      
      codegen_value create_function(llvm::Value *& val, llvm::Module *module, llvm::IRBuilder<> &builder);

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
