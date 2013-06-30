#ifndef GD_RL_AST_DISTRIBUTION_HPP
#define GD_RL_AST_DISTRIBUTION_HPP

#include "compiler/ast/global.hpp"
#include "compiler/ast/expression.hpp"
#include "compiler/ast/typename.hpp"

namespace raytrace {

  namespace ast {

    //A parameter to a distribution.
    struct distribution_parameter {
      std::string name;
      type_expr_ptr type;
    };
    
    /* Defines a new distribution function. */
    class distribution : public global_declaration {
    public:

      distribution(parser_state *st, const std::string &name,
		   const std::vector<expression_ptr> &flags,
		   const std::vector<distribution_parameter> &params,
		   const std::vector<global_declaration_ptr> &internal_decl,
		   unsigned int line_no, unsigned int column_no);
      virtual ~distribution() {}

      virtual codegen_value codegen(llvm::Module *module, llvm::IRBuilder<> &builder);

      std::string evaluator_name(const std::string &n);
      
    private:

      std::string name;
      std::vector<expression_ptr> flags;
      std::vector<distribution_parameter> params;
      std::vector<global_declaration_ptr> internal_decl;
      
      typecheck_vector get_parameter_types();

      llvm::StructType *getParameterType(const std::vector<type_spec> &param_types);

      codegen_value createConstructor(llvm::Module *module, llvm::IRBuilder<> &builder,
				      const std::string &ctor_name,
				      llvm::Type *parameter_type, const std::vector<type_spec> &param_type_list,
				      llvm::Value *eval, llvm::Value *sample, llvm::Value *pdf, llvm::Value *emit,
				      llvm::Function *dtor);
      
      llvm::Function *createDestructor(llvm::Module *module, llvm::IRBuilder<> &builder,
				       llvm::Type *param_ty, const std::vector<type_spec> &param_type_list);

      //evaluates all flag expressions (for use in the constructor).
      codegen_value codegen_all_flags(llvm::Module *module, llvm::IRBuilder<> &builder);
      
      //Checks for a function using the given key.
      codegen_value check_for_function(const function_key &key,
				       const std::vector<bool> &output_args,
				       bool use_default = false);
      
      //Creates a wrapper function that can be called from C++ (aggregate types are passed as pointers).
      struct wrapper_argument {
	type_spec ty;
	bool is_output;
      };

      llvm::Value *create_wrapper(llvm::Function *func, const std::string &name,
				  llvm::Type *parameter_type,
				  const std::vector<wrapper_argument> &arguments,
				  llvm::Type *return_type,
				  bool last_arg_as_return, const type_spec &return_arg_type,
				  llvm::Module *module, llvm::IRBuilder<> &builder);

      //Functions looking for and wrapping the distribution's main functions.
      codegen_value check_for_evaluate();
      llvm::Value *create_evaluator(llvm::Function *eval, llvm::Type *parameter_type,
				    llvm::Module *module, llvm::IRBuilder<> &builder);

      codegen_value check_for_sample();
      llvm::Value *create_sampler(llvm::Function *sample, llvm::Type *parameter_type,
				     llvm::Module *module, llvm::IRBuilder<> &builder);

      codegen_value check_for_pdf();
      llvm::Value *create_pdf(llvm::Function *pdf, llvm::Type *parameter_type,
			      llvm::Module *module, llvm::IRBuilder<> &builder);

      codegen_value check_for_emission();
      llvm::Value *create_emission(llvm::Function *emit, llvm::Type *parameter_type,
				   llvm::Module *module, llvm::IRBuilder<> &builder);
      

    };

  };

};

#endif
