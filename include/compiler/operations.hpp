#ifndef RT_OPERATIONS_HPP
#define RT_OPERATIONS_HPP

#include "compiler/ast/ast.hpp"
#include "llvm/IRBuilder.h"

namespace raytrace {
  
  //Add

  //Returns the result type of the addition operation.
  type_spec get_add_result_type(const ast::expression_ptr &lhs, const ast::expression_ptr &rhs);

  //Performs any type-checking and conversions to generate instructions adding the two expressions.
  llvm::Value *generate_add(ast::expression_ptr &lhs, ast::expression_ptr &rhs,
			    llvm::Module *module, llvm::IRBuilder<> &builder);

  //Less-Than

  llvm::Value *generate_less_than(ast::expression_ptr &lhs, ast::expression_ptr &rhs,
				  llvm::Module *module, llvm::IRBuilder<> &builder);
  

  //Helpers

  llvm::Value *llvm_builtin_binop(const std::string &func_name, llvm::Type *type, llvm::Value *lhs, llvm::Value *rhs,
				  llvm::Module *module, llvm::IRBuilder<> &builder);
};

#endif
