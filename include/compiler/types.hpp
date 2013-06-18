#ifndef RT_TYPES_HPP
#define RT_TYPES_HPP

#include <string>
#include "llvm/DerivedTypes.h"
#include "llvm/IRBuilder.h"
#include "llvm/Module.h"

#include <stdexcept>
#include <boost/unordered_map.hpp>

#include "compiler/errors.hpp"
#include "compiler/value.hpp"

namespace raytrace {
  
  /* Describes an instance of a type. */
  class type;
  //typedef std::shared_ptr<type> type_spec;
  typedef type* type_spec;

  std::size_t hash_value(const type_spec &ts);

  typedef codegen<type_spec, compile_error>::value typecheck_value;
  typedef codegen<type_spec, compile_error>::vector typecheck_vector;
  
  typedef codegen<value, compile_error>::value code_value;
  typedef boost::tuple<value, type_spec> typed_value;
  typedef codegen<typed_value, compile_error>::value typed_value_container;

  typedef codegen<typed_value, compile_error>::vector typed_value_vector;

  typedef codegen<empty_type, compile_error>::vector void_vector;
  
  /* A table of types. */
  class type_table {
  public:

    typedef std::unique_ptr<type> type_ptr;
    
    type *operator[](const std::string &key) { return entries[key].get(); }
    type *at(const std::string &key) { return entries.at(key).get(); }
    bool has_type(const std::string &key) { return entries.find(key) != entries.end(); }

    type_ptr &entry(const std::string &key) { return entries[key]; }
    
    type *get_array(const type_spec &base, unsigned int N);
    type *get_array_ref(const type_spec &base);

    type *add_nameless(std::unique_ptr<type> &&ptr);

  private:
    
    boost::unordered_map< std::string, std::unique_ptr<type> > entries;
    boost::unordered_map< std::string, std::unique_ptr<type> > array_types;

    std::vector<std::unique_ptr<type>> nameless_types;

  };

  void initialize_types(type_table &tt);

  class type_conversion_table;
  
  /* Describes a type in the Gideon Render Language. */
  class type {
  public:
    
    //type information
    const std::string name, type_id;
    const bool is_differentiable;

    virtual bool is_iterator() const { return false; }
    virtual bool is_array() const { return false; }
    
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
				   llvm::Module *module, llvm::IRBuilder<> &builder) const { return errors::make_error<errors::error_message>("Invalid cast", 0, 0); }

    //allocates memory for a new value of this type, returning a pointer.
    virtual llvm::Value *allocate(llvm::Module *module, llvm::IRBuilder<> &builder) const;

    //dereferences a pointer to a value of this type.
    virtual llvm::Value *load(llvm::Value *ptr, llvm::Module *module, llvm::IRBuilder<> &builder) const;

    //stores the given value at the location provided by the pointer.
    virtual void store(llvm::Value *value, llvm::Value *ptr, llvm::Module *module, llvm::IRBuilder<> &builder) const;

    //destruction/copy
    virtual typed_value_container initialize(llvm::Module *module, llvm::IRBuilder<> &builder) const {
      return typed_value(static_cast<llvm::Value*>(nullptr), types->at(name));
    }
    virtual codegen_void destroy(llvm::Value *value, llvm::Module *module, llvm::IRBuilder<> &builder) { return empty_type(); }
    
    virtual llvm::Value *copy(llvm::Value *value, llvm::Module *module, llvm::IRBuilder<> &builder) { return value; }
    
    virtual typed_value_container create(llvm::Module *module, llvm::IRBuilder<> &builder, typed_value_vector &args,
					 const type_conversion_table &conversions) const;

    virtual llvm::Type *llvm_type() const = 0;
    
    //Returns the llvm type corresponding to a pointer to this type.
    virtual llvm::Type *llvm_ptr_type() const;

    //field access
    virtual typecheck_value field_type(const std::string &field) const;
    virtual typed_value_container access_field(const std::string &field, llvm::Value *value,
					       llvm::Module *module, llvm::IRBuilder<> &builder) const;

    virtual typed_value_container access_field_ptr(const std::string &field, llvm::Value *value_ptr,
						   llvm::Module *module, llvm::IRBuilder<> &builder) const;

    //array element access
    virtual type_spec element_type() const;

    virtual typed_value_container access_element(llvm::Value *value, llvm::Value *elem_idx,
						 llvm::Module *module, llvm::IRBuilder<> &builder) const;
    
    virtual typed_value_container access_element_ptr(llvm::Value *value_ptr, llvm::Value *elem_idx,
						     llvm::Module *module, llvm::IRBuilder<> &builder) const;
    
  protected:

    type_table *types;
    
    type(type_table *types,
	 const std::string &name, const std::string &type_id, bool is_differentiable = false) : types(types), name(name), type_id(type_id), is_differentiable(is_differentiable) { }
    
    compile_error arg_count_mismatch(unsigned int expected, unsigned int found) const;
    
  };

};

#endif
