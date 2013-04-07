#ifndef GD_SL_PRIMITIVE_TYPES_HPP
#define GD_SL_PRIMITIVE_TYPES_HPP

#include "compiler/types.hpp"

namespace raytrace {

  class void_type : public type {
  public:

    void_type(type_table *types) : type(types, "void", "v") { }
    virtual llvm::Type *llvm_type() const;

  };

  class bool_type : public type {
  public:

    bool_type(type_table *types) : type(types, "bool", "b") { }
    virtual llvm::Type *llvm_type() const;

  };

  class int_type : public type {
  public:

    int_type(type_table *types) : type(types, "int", "i") { }
    virtual llvm::Type *llvm_type() const;
    
    //operations
    virtual codegen_value op_add(llvm::Module *module, llvm::IRBuilder<> &builder,
				 codegen_value &lhs, codegen_value &rhs) const;

    virtual codegen_value op_less(llvm::Module *module, llvm::IRBuilder<> &builder,
				  codegen_value &lhs, codegen_value &rhs) const;
  };

  class float_type : public type {
  public:

    float_type(type_table *types) : type(types, "float", "f", true) { }
    virtual llvm::Type *llvm_type() const;

    //operations
    virtual codegen_value op_add(llvm::Module *module, llvm::IRBuilder<> &builder,
				 codegen_value &lhs, codegen_value &rhs) const;

    virtual codegen_value op_less(llvm::Module *module, llvm::IRBuilder<> &builder,
				  codegen_value &lhs, codegen_value &rhs) const;
  };

  //Strings
  class string_type : public type {
  public:

    string_type(type_table *types);

    virtual llvm::Type *llvm_type() const;
    
    virtual codegen_value initialize(llvm::Module *module, llvm::IRBuilder<> &builder) const;
    virtual llvm::Value *copy(llvm::Value *value, llvm::Module *module, llvm::IRBuilder<> &builder);

    virtual codegen_void destroy(llvm::Value *value, llvm::Module *module, llvm::IRBuilder<> &builder);
    
  private:

    llvm::Type *str_type_value;

  };

  //Vector Types

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

  class scene_ptr_type : public type {
  public:

    scene_ptr_type(type_table *types) : type(types, "scene_ptr", "sp") { }
    virtual llvm::Type *llvm_type() const;

  };

};

#endif
