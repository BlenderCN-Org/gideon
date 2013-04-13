#ifndef GD_SL_VECTOR_TYPES_HPP
#define GD_SL_VECTOR_TYPES_HPP

#include "compiler/types.hpp"

namespace raytrace {
  
  class float4_type : public type {
  public:
    
    float4_type(type_table *types);
    virtual llvm::Type *llvm_type() const;

    virtual codegen_value create(llvm::Module *module, llvm::IRBuilder<> &builder, typed_value_vector &args) const;

    virtual codegen_value op_add(llvm::Module *module, llvm::IRBuilder<> &builder,
				 codegen_value &lhs, codegen_value &rhs) const;

  private:

    llvm::Type *type_value;

  };

  class float3_type : public type {
  public:
    
    float3_type(type_table *types);
    virtual llvm::Type *llvm_type() const;
    
    virtual codegen_value create(llvm::Module *module, llvm::IRBuilder<> &builder, typed_value_vector &args) const;
    
    virtual codegen_value op_add(llvm::Module *module, llvm::IRBuilder<> &builder,
				 codegen_value &lhs, codegen_value &rhs) const;

  private:

    llvm::Type *type_value;

  };

  class float2_type : public type {
  public:
    
    float2_type(type_table *types);
    virtual llvm::Type *llvm_type() const;
    
    virtual codegen_value create(llvm::Module *module, llvm::IRBuilder<> &builder, typed_value_vector &args) const;
    
    virtual codegen_value op_add(llvm::Module *module, llvm::IRBuilder<> &builder,
				 codegen_value &lhs, codegen_value &rhs) const;

  private:

    llvm::Type *type_value;

  };

};

#endif
