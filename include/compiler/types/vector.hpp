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
    
    virtual code_value create(llvm::Module *module, llvm::IRBuilder<> &builder, typed_value_vector &args,
			      const type_conversion_table &conversions) const;
    
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
