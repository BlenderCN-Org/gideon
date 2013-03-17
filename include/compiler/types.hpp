#ifndef RT_TYPES_HPP
#define RT_TYPES_HPP

#include <string>
#include "llvm/DerivedTypes.h"
#include "llvm/IRBuilder.h"
#include "llvm/Module.h"

namespace raytrace {
  
  /* Enum defining the various types supported by the shader language. */
  enum type_code {
    BOOL,
    INT,
    FLOAT,
    FLOAT4,
    RAY,
    VOID,
    OTHER
  };

  /* Returns the type code equivalent to this C++ type. */
  template<typename T>
  type_code get_type_code();

  typedef void (*llvm_gen_destroy_func)(llvm::Value *addr, llvm::Module *module, llvm::IRBuilder<> &builder);

  /* Provides info about a particular built-in type. */
  struct type_traits {
    std::string name, code;
    type_code type;

    bool is_primitive;
    bool is_scalar;
    size_t size;
    
    llvm_gen_destroy_func destroy;
  };

  /* Returns the type_traits struct associated with this type code. */
  template<type_code T>
  type_traits get_type_traits();
  type_traits get_type_traits(type_code t);
  
  /* Type specifier used for variable declarations, function arguments/return types, etc. */
  struct type_spec {
    type_code t;

    type_spec() : t(type_code::OTHER) {}
    type_spec(type_code t) : t(t) {}
    
    bool base_equal(const type_spec &rhs);

    bool operator==(const type_spec &rhs);
    bool operator!=(const type_spec &rhs) { return !(*this == rhs); }
    llvm::Type *llvm_type() const;
    
    void destroy(llvm::Value *addr, llvm::Module *module, llvm::IRBuilder<> &builder);

    std::string get_arg_code() const;
  };

  llvm::Type *get_llvm_primitive_type(const type_code t);
  llvm::Type *get_llvm_tuple_type(const type_code t, unsigned int n);
  llvm::Type *get_llvm_chunk_type(const type_code t);
  llvm::Type *get_llvm_type(const type_code t);
  
  /* Type constructors */
  
  llvm::Value *make_llvm_float4(llvm::Module *module, llvm::IRBuilder<> &builder,
				llvm::Value *x, llvm::Value *y, llvm::Value *z, llvm::Value *w);
};

#endif
