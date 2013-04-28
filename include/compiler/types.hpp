#ifndef RT_TYPES_HPP
#define RT_TYPES_HPP

#include <string>
#include "llvm/DerivedTypes.h"
#include "llvm/IRBuilder.h"
#include "llvm/Module.h"

#include <stdexcept>
#include <boost/unordered_map.hpp>

#include "compiler/errors.hpp"

namespace raytrace {
  
  /* Describes an instance of a type. */
  class type;
  typedef std::shared_ptr<type> type_spec;
  std::size_t hash_value(const type_spec &ts);

  typedef codegen<type_spec, compile_error>::value typecheck_value;
  typedef codegen<type_spec, compile_error>::vector typecheck_vector;

  typedef errors::argument_value_join<codegen_value, typecheck_value>::result_value_type typed_value;
  typedef errors::argument_value_join<codegen_value, typecheck_value>::result_type typed_value_container;
  typedef codegen<typed_value, compile_error>::vector typed_value_vector;

  /* A table of types. */
  typedef boost::unordered_map< std::string, std::shared_ptr<type> > type_table;
  void initialize_types(type_table &tt);
  
  /* Describes a type in the Gideon Render Language. */
  class type {
  public:
    
    //type information
    const std::string name, type_id;
    const bool is_differentiable;

    virtual bool is_iterator() const { return false; }
    
    bool operator==(const type &rhs) const { return type_id == rhs.type_id; }
    bool operator!=(const type &rhs) const { return !(*this == rhs); }

    //casting
    virtual bool can_cast_to(const type &other, /* out */ int &cost) const {
      if (*this == other) {
	cost = 0;
	return true;
      }

      cost = std::numeric_limits<int>::max();
      return false;
    };
    virtual codegen_value gen_cast(const type &other, llvm::Value *value,
				   llvm::Module *module, llvm::IRBuilder<> &builder) const { return compile_error("Invalid cast"); }

    //destruction/copy
    virtual codegen_value initialize(llvm::Module *module, llvm::IRBuilder<> &builder) const { return nullptr; }
    virtual codegen_void destroy(llvm::Value *value, llvm::Module *module, llvm::IRBuilder<> &builder) { return nullptr; }
    
    virtual llvm::Value *copy(llvm::Value *value, llvm::Module *module, llvm::IRBuilder<> &builder) { return value; }
    
    virtual codegen_value create(llvm::Module *module, llvm::IRBuilder<> &builder, typed_value_vector &args) const;

    virtual llvm::Type *llvm_type() const = 0;

    //field access
    virtual typecheck_value field_type(const std::string &field) const;
    virtual codegen_value access_field(const std::string &field, llvm::Value *value,
				       llvm::Module *module, llvm::IRBuilder<> &builder) const;
    virtual codegen_value access_field_ptr(const std::string &field, llvm::Value *value_ptr,
					   llvm::Module *module, llvm::IRBuilder<> &builder) const;

    //operations
    virtual codegen_value op_add(llvm::Module *module, llvm::IRBuilder<> &builder,
				 codegen_value &lhs, codegen_value &rhs) const;

    virtual codegen_value op_sub(llvm::Module *module, llvm::IRBuilder<> &builder,
				 codegen_value &lhs, codegen_value &rhs) const;

    virtual codegen_value op_mul(llvm::Module *module, llvm::IRBuilder<> &builder,
				 codegen_value &lhs, codegen_value &rhs) const;
    
    virtual codegen_value op_div(llvm::Module *module, llvm::IRBuilder<> &builder,
				 codegen_value &lhs, codegen_value &rhs) const;
    
    
    virtual codegen_value op_less(llvm::Module *module, llvm::IRBuilder<> &builder,
				  codegen_value &lhs, codegen_value &rhs) const;
    
    

  protected:

    type_table *types;
    
    type(type_table *types,
	 const std::string &name, const std::string &type_id, bool is_differentiable = false) : types(types), name(name), type_id(type_id), is_differentiable(is_differentiable) { }
    
    compile_error arg_count_mismatch(unsigned int expected, unsigned int found) const;
    
  };

};

#endif
