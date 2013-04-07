#ifndef RT_AST_EXPRESSION_HPP
#define RT_AST_EXPRESSION_HPP

#include "llvm/DerivedTypes.h"
#include "llvm/IRBuilder.h"
#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/Analysis/Verifier.h"

#include "compiler/types.hpp"
#include "compiler/ast/node.hpp"

#include <memory>
#include <stdexcept>

namespace raytrace {

  namespace ast {
    
    /* Base class for expressions */
    class expression : public ast_node {
    public:

      expression(parser_state *st) : ast_node(st), result_type(st->types["void"]) {}
      expression(parser_state *st, const type_spec &rt) : ast_node(st), result_type(rt) {}
      
      virtual ~expression() {}
      virtual codegen_value codegen(llvm::Module *module, llvm::IRBuilder<> &builder) = 0;

      virtual codegen_value codegen_ptr(llvm::Module *module, llvm::IRBuilder<> &builder);
      virtual bool has_ptr() { return false; } //returns true if this expression has an address
      
      virtual type_spec typecheck() = 0;
      typecheck_value typecheck_safe();
      
    protected:

      type_spec result_type;
      
    };

    typedef std::shared_ptr<expression> expression_ptr;

    /* A binary operation */
    class binary_expression : public expression {
    public:
      
      binary_expression(parser_state *st, const std::string &op, const expression_ptr &lhs, const expression_ptr &rhs);
      virtual ~binary_expression() {}
      
      virtual codegen_value codegen(llvm::Module *module, llvm::IRBuilder<> &builder);

      virtual type_spec typecheck();
      
    private:
      
      std::string op;
      expression_ptr lhs, rhs;
      
    };
  
  };
};

#endif
