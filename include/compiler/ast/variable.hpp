#ifndef RT_AST_VARIABLE_HPP
#define RT_AST_VARIABLE_HPP

#include "compiler/types.hpp"
#include "compiler/ast/expression.hpp"
#include "compiler/ast/statement.hpp"
#include "compiler/ast/global.hpp"
#include "compiler/symboltable.hpp"

#include "compiler/errors.hpp"

namespace raytrace {
  
  namespace ast {
    
    /* Declares a new variable. */
    class variable_decl : public statement {
    public:

      variable_decl(parser_state *st, const std::string &name,
		    const type_spec &type, const expression_ptr &init,
		    unsigned int line_no, unsigned int column_no);
      virtual ~variable_decl() {}

      virtual codegen_value codegen(llvm::Module *module, llvm::IRBuilder<> &builder);
      
    private:
      
      std::string name;
      type_spec type;
      expression_ptr initializer;
      
    };
    
    /* Declares a new global variable. */
    
    class global_variable_decl : public global_declaration {
    public:
      
      global_variable_decl(parser_state *st,
			   const std::string &name, const type_spec &type);
      virtual ~global_variable_decl() { }

      virtual codegen_value codegen(llvm::Module *module, llvm::IRBuilder <> &builder);

    private:
      
      std::string name;
      type_spec type;

    };

    /* A reference to a variable. */
    class variable_ref : public expression {
    public:

      variable_ref(parser_state *st, const std::string &name);
      virtual ~variable_ref() {}
      
      virtual codegen_value codegen(llvm::Module *module, llvm::IRBuilder<> &builder);
      virtual codegen_value codegen_ptr(llvm::Module *module, llvm::IRBuilder<> &builder);
      virtual type_spec typecheck();
      
    private:
      
      std::string name;
      codegen_value lookup_var();

    };

    /* Assigns an expression to an lvalue. */
    class assignment : public expression {
    public:

      assignment(parser_state *st, const expression_ptr &lhs, const expression_ptr &rhs);
      virtual ~assignment() {}

      virtual codegen_value codegen(llvm::Module *module, llvm::IRBuilder<> &builder);
      virtual type_spec typecheck();
      
    private:

      expression_ptr lhs, rhs;
      
    };

    /* Type Constructors. */
    class type_constructor : public expression {
    public:

      type_constructor(parser_state *st, const type_spec &type, const std::vector<expression_ptr> &args);
      ~type_constructor() {}

      virtual codegen_value codegen(llvm::Module *module, llvm::IRBuilder<> &builder);
      virtual type_spec typecheck() { return type; }

    private:

      type_spec type;
      std::vector<expression_ptr> args;
      
      codegen_value get_argval(int i, llvm::Module *module, llvm::IRBuilder<> &builder);
      typecheck_value get_argtype(int i);
    };

    /* Access the field of a vector or struct (as the result of an expression). */
    class field_selection : public expression {
    public:

      field_selection(parser_state *st, const std::string &field, const expression_ptr &expr,
		      unsigned int line_no, unsigned int column_no);
      virtual ~field_selection() {}

      virtual codegen_value codegen(llvm::Module *module, llvm::IRBuilder<> &builder);
      virtual codegen_value codegen_ptr(llvm::Module *module, llvm::IRBuilder<> &builder);
      
      virtual type_spec typecheck();
      
    private:

      std::string field;
      expression_ptr expr;
      
    };

  };

};

#endif
