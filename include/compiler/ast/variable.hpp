#ifndef RT_AST_VARIABLE_HPP
#define RT_AST_VARIABLE_HPP

#include "compiler/types.hpp"
#include "compiler/ast/expression.hpp"
#include "compiler/ast/statement.hpp"
#include "compiler/symboltable.hpp"

namespace raytrace {
  
  namespace ast {

    /* Base class for expressions returning values that can be the target of an assignment. */
    class lvalue : public expression {
    public:
      
      lvalue(const type_code &type) : expression(type) {}
      virtual ~lvalue() {}

    };

    typedef std::shared_ptr<lvalue> lvalue_ptr;
    
    /* Declares a new variable. */
    class variable_decl : public statement {
    public:

      variable_decl(var_symbol_table *symtab, const std::string &name,
		    const type_spec &type, const expression_ptr &init);
      virtual ~variable_decl() {}

      virtual llvm::Value *codegen(llvm::Module *module, llvm::IRBuilder<> &builder);
      
    private:

      var_symbol_table *symtab;
      std::string name;
      type_spec type;
      expression_ptr initializer;
      
    };

    /* A lvalue variable reference. */
    class variable_lvalue : public lvalue {
    public:

      variable_lvalue(var_symbol_table *symtab, const std::string &name);
      virtual ~variable_lvalue() {}
      
      virtual llvm::Value *codegen(llvm::Module *module, llvm::IRBuilder<> &builder);
      virtual type_spec typecheck();
      
    private:
      
      std::string name;
      var_symbol_table *symtab;
      
    };

    /* A reference to a variable or field. */
    class variable_ref : public expression {
    public:

      variable_ref(const lvalue_ptr &lval);
      virtual ~variable_ref() {}
      
      virtual llvm::Value *codegen(llvm::Module *module, llvm::IRBuilder<> &builder);
      virtual llvm::Value *codegen_ptr(llvm::Module *module, llvm::IRBuilder<> &builder);
      virtual bool has_ptr() { return true; }
      
      virtual type_spec typecheck();
      
    private:
      
      lvalue_ptr lval_ref;
      
    };

    /* Assigns an expression to an lvalue. */
    class assignment : public expression {
    public:

      assignment(const lvalue_ptr &lhs, const expression_ptr &rhs);
      virtual ~assignment() {}

      virtual llvm::Value *codegen(llvm::Module *module, llvm::IRBuilder<> &builder);
      virtual type_spec typecheck();

    private:

      lvalue_ptr lhs;
      expression_ptr rhs;
      
    };

    /* Type Constructors. */
    class type_constructor : public expression {
    public:

      type_constructor(const type_spec &type, const std::vector<expression_ptr> &args);
      ~type_constructor() {}

      virtual llvm::Value *codegen(llvm::Module *module, llvm::IRBuilder<> &builder);
      virtual type_spec typecheck() { return type; }

    private:

      type_spec type;
      std::vector<expression_ptr> args;

      void check_args(const std::vector<type_spec> &expected);
      llvm::Value *get_argval(int i, llvm::Module *module, llvm::IRBuilder<> &builder);
    };
  };

};

#endif
