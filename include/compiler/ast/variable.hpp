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

    /* Base class for expressions returning values that can be the target of an assignment. */
    class lvalue : public expression {
    public:
      
      lvalue(parser_state *st, const type_spec &type) : expression(st, type) {}
      virtual ~lvalue() {}

    };

    typedef std::shared_ptr<lvalue> lvalue_ptr;
    
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

    /* A lvalue variable reference. */
    class variable_lvalue : public lvalue {
    public:

      variable_lvalue(parser_state *st, const std::string &name);
      virtual ~variable_lvalue() {}
      
      virtual codegen_value codegen(llvm::Module *module, llvm::IRBuilder<> &builder);
      virtual type_spec typecheck();
      
    private:
      
      std::string name;
      
    };

    /* A reference to a variable or field. */
    class variable_ref : public expression {
    public:

      variable_ref(parser_state *st, const lvalue_ptr &lval);
      virtual ~variable_ref() {}
      
      virtual codegen_value codegen(llvm::Module *module, llvm::IRBuilder<> &builder);
      virtual codegen_value codegen_ptr(llvm::Module *module, llvm::IRBuilder<> &builder);
      virtual bool has_ptr() { return true; }
      
      virtual type_spec typecheck();
      
    private:
      
      lvalue_ptr lval_ref;
      
    };

    /* Assigns an expression to an lvalue. */
    class assignment : public expression {
    public:

      assignment(parser_state *st, const lvalue_ptr &lhs, const expression_ptr &rhs);
      virtual ~assignment() {}

      virtual codegen_value codegen(llvm::Module *module, llvm::IRBuilder<> &builder);
      virtual type_spec typecheck();
      
    private:

      lvalue_ptr lhs;
      expression_ptr rhs;
      
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
  };

};

#endif
