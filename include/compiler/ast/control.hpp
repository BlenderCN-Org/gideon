#ifndef RT_AST_CONTROL_HPP
#define RT_AST_CONTROL_HPP

#include "llvm/DerivedTypes.h"
#include "llvm/IRBuilder.h"
#include "llvm/LLVMContext.h"
#include "llvm/Module.h"

#include "compiler/types.hpp"
#include "compiler/gen_state.hpp"

#include "compiler/ast/statement.hpp"

namespace raytrace {
  
  namespace ast {

    /* if-else condition */
    class conditional_statement : public statement {
    public:

      conditional_statement(const expression_ptr &cond,
			    const statement_ptr &if_branch, const statement_ptr &else_branch);
      virtual ~conditional_statement() {}

      virtual llvm::Value *codegen(llvm::Module *module, llvm::IRBuilder<> &builder);

    private:

      expression_ptr cond;
      statement_ptr if_branch, else_branch;

    };

    /* for-loop construct */
    class for_loop_statement : public statement {
    public:

      for_loop_statement(const statement_ptr &init,
			 const expression_ptr &cond,
			 const expression_ptr &after,
			 const statement_ptr &body,
			 var_symbol_table *variables, func_symbol_table *functions,
			 control_state *control);

      virtual ~for_loop_statement() {}

      virtual llvm::Value *codegen(llvm::Module *module, llvm::IRBuilder<> &builder);

    private:

      statement_ptr init, body;
      expression_ptr cond, after;
      
      var_symbol_table *variables;
      func_symbol_table *functions;
      control_state *control;
    };

    /* Break Statement */
    class break_statement : public statement {
    public:

      break_statement(control_state *control);
      virtual ~break_statement() {}

      virtual llvm::Value *codegen(llvm::Module *module, llvm::IRBuilder<> &builder);
      virtual bool is_terminator() { return true; }

    private:

      control_state *control;

    };

   /* Continue Statement */
    class continue_statement : public statement {
    public:

      continue_statement(control_state *control);
      virtual ~continue_statement() {}

      virtual llvm::Value *codegen(llvm::Module *module, llvm::IRBuilder<> &builder);
      virtual bool is_terminator() { return true; }

    private:

      control_state *control;

    };

  };

};

#endif
