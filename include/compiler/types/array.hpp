/*

  Copyright 2013 Curtis Andrus

  This file is part of Gideon.

  Gideon is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  Gideon is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with Gideon.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef GD_RL_ARRAY_TYPES_HPP
#define GD_RL_ARRAY_TYPES_HPP

#include "compiler/types.hpp"

namespace raytrace {

  /*
    Defines a constant-sized array type.
   */
  class array_type : public type {
  public:

    array_type(type_table *types,
	       const type_spec &base, unsigned int N);

    virtual bool is_array() const { return true; }

    static std::string type_name(const type_spec &base, unsigned int N);
    static std::string type_code(const type_spec &base, unsigned int N);

    virtual llvm::Type *llvm_type() const;
    
    virtual code_value initialize(llvm::Module *module, llvm::IRBuilder<> &builder) const;
    virtual codegen_void destroy(llvm::Value *value, llvm::Module *module, llvm::IRBuilder<> &builder);

    virtual code_value create(llvm::Module *module, llvm::IRBuilder<> &builder,
			      typed_value_vector &args, const type_conversion_table &conversions) const;

    //array element access
    virtual type_spec element_type() const { return base; }

    //arrays have a 'length' field
    virtual typed_value_container access_field(const std::string &field, llvm::Value *value,
					       llvm::Module *module, llvm::IRBuilder<> &builder) const;

    virtual typed_value_container access_element(llvm::Value *value, llvm::Value *elem_idx,
						 llvm::Module *module, llvm::IRBuilder<> &builder) const;
    
    virtual typed_value_container access_element_ptr(llvm::Value *value_ptr, llvm::Value *elem_idx,
						     llvm::Module *module, llvm::IRBuilder<> &builder) const;



    
  private:
    
    type_spec base;
    unsigned int N;
    llvm::Type *arr_type;
    
  };

  /*
    A reference to an array of any size.
    This is used as a function parameter to allow
    functions to support arbitrarily-sized arrays.

    Basically a struct containing: { i32 size, arr_type* data }
  */
  class array_reference_type : public type {
  public:

    array_reference_type(type_table *types, const type_spec &base);
    virtual bool is_array() const { return true; }
    
    static std::string type_name(const type_spec &base);
    static std::string type_code(const type_spec &base);

    virtual llvm::Type *llvm_type() const;
    
    virtual code_value initialize(llvm::Module *module, llvm::IRBuilder<> &builder) const;
    
    //array element access
    virtual type_spec element_type() const { return base; }

    //arrays have a 'length' field
    virtual typed_value_container access_field(const std::string &field, llvm::Value *value,
					       llvm::Module *module, llvm::IRBuilder<> &builder) const;

    virtual typed_value_container access_element(llvm::Value *value, llvm::Value *elem_idx,
						 llvm::Module *module, llvm::IRBuilder<> &builder) const;
    
    virtual typed_value_container access_element_ptr(llvm::Value *value_ptr, llvm::Value *elem_idx,
						     llvm::Module *module, llvm::IRBuilder<> &builder) const;
  private:

    type_spec base;
    llvm::Type *ref_type;
    
  };

};

#endif
