#ifndef GD_RL_AST_DISTRIBUTION_HPP
#define GD_RL_AST_DISTRIBUTION_HPP

#include "compiler/ast/global.hpp"

namespace raytrace {

  namespace ast {
    
    /* Defines a new distribution function. */
    class distribution : public global_declaration {
    public:

      distribution(parser_state *st, const std::string &name,
		   const std::vector<function_argument> &params,
		   const std::vector<global_declaration_ptr> &internal_decl,
		   unsigned int line_no, unsigned int column_no);
      virtual ~distribution() {}

      virtual codegen_value codegen(llvm::Module *module, llvm::IRBuilder<> &builder);

      std::string evaluator_name(const std::string &n) const;
      
    private:

      std::string name;
      std::vector<function_argument> params;
      std::vector<global_declaration_ptr> internal_decl;
      llvm::Type *param_type;

      llvm::StructType *getParameterType();
      llvm::Function *createConstructor(llvm::Module *module, llvm::IRBuilder<> &builder, llvm::Function *eval, llvm::Function *dtor);
      llvm::Function *createDestructor(llvm::Module *module, llvm::IRBuilder<> &builder);
      llvm::Function *createEvaluator(llvm::Function *eval, llvm::Module *module, llvm::IRBuilder<> &builder);
    };

  };

};

#endif
