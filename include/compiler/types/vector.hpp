#ifndef GD_SL_VECTOR_TYPES_HPP
#define GD_SL_VECTOR_TYPES_HPP

#include "compiler/types.hpp"

namespace raytrace {
  
  class floatN_type : public type {
  public:
    
    floatN_type(type_table *types, unsigned int N);
    virtual llvm::Type *llvm_type() const;
    
    virtual typecheck_value field_type(const std::string &field) const;
    virtual typed_value_container access_field(const std::string &field, llvm::Value *value,
					       llvm::Module *module, llvm::IRBuilder<> &builder) const;
    virtual typed_value_container access_field_ptr(const std::string &field, llvm::Value *value_ptr,
						   llvm::Module *module, llvm::IRBuilder<> &builder) const;
    
    virtual typed_value_container create(llvm::Module *module, llvm::IRBuilder<> &builder, typed_value_vector &args) const;
    
    static std::string type_name(unsigned int N);
    static std::string type_code(unsigned int N);

  private:

    unsigned int N;
    llvm::Type *type_value;
    
    bool get_element_idx(char c, /* out */ unsigned int &idx) const;
    compile_error invalid_field(const std::string &field) const;
  };


};

#endif
