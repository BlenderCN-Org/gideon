#ifndef GD_SL_SPECIAL_TYPES_HPP
#define GD_SL_SPECIAL_TYPES_HPP

#include "compiler/types.hpp"

namespace raytrace {
  
  class ray_type : public type {
  public:
    
    ray_type(type_table *types) : type(types, "ray", "r") { }
    virtual llvm::Type *llvm_type() const;
    
  };

  class intersection_type : public type {
  public:

    intersection_type(type_table *types) : type(types, "isect", "is") { }
    virtual llvm::Type *llvm_type() const;

  };

  class light_type : public type {
  public:

    light_type(type_table *types) : type(types, "light", "lt") { }
    virtual llvm::Type *llvm_type() const;

  };

};

#endif
