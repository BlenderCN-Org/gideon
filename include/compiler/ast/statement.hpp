#ifndef RT_AST_STATEMENT_HPP
#define RT_AST_STATEMENT_HPP

#include "llvm/DerivedTypes.h"
#include "llvm/IRBuilder.h"
#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/Analysis/Verifier.h"

#include "compiler/symboltable.hpp"
#include "compiler/ast/expression.hpp"

#include <memory>
#include <vector>

namespace raytrace {

  namespace ast {

    /* Generic statement node. */
    class statement : public ast_node {
    public:

      statement(parser_state *st) : ast_node(st) {}
      virtual ~statement() {}

      virtual codegen_value codegen(llvm::Module *module, llvm::IRBuilder<> &builder) = 0;
      virtual bool is_terminator() { return false; }
    };

    typedef std::shared_ptr<statement> statement_ptr;

    /* Statement containing an expression. */
    class expression_statement : public statement {
    public:

      expression_statement(parser_state *st, const expression_ptr &expr);
      virtual ~expression_statement() {}

      virtual codegen_value codegen(llvm::Module *module, llvm::IRBuilder<> &builder);
      
    private:
      
      expression_ptr expr;

    };

    /* Lists of statements. */
    class statement_list {
    public:

      statement_list(const std::vector<statement_ptr> &statements);
      codegen_void codegen(llvm::Module *module, llvm::IRBuilder<> &builder);

    private:

      std::vector<statement_ptr> statements;

    };

    /* Statements executing within a given scope. */
    class scoped_statement : public statement {
    public:

      scoped_statement(parser_state *st, const statement_list &stmt_list);
      virtual ~scoped_statement() { }

      virtual codegen_value codegen(llvm::Module *module, llvm::IRBuilder<> &builder);

    private:
      
      statement_list statements;
      
    };

  };

};

#endif