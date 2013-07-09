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

  };

  class float_type : public type {
  public:

    float_type(type_table *types) : type(types, "float", "f", true) { }
    virtual llvm::Type *llvm_type() const;
    
  };

  //Strings
  class string_type : public type {
  public:

    string_type(type_table *types);

    virtual llvm::Type *llvm_type() const;
    
    virtual typed_value_container initialize(llvm::Module *module, llvm::IRBuilder<> &builder) const;
    virtual llvm::Value *copy(llvm::Value *value, llvm::Module *module, llvm::IRBuilder<> &builder);

    virtual codegen_void destroy(llvm::Value *value, llvm::Module *module, llvm::IRBuilder<> &builder);
    
  private:

    llvm::Type *str_type_value;

  };

  class scene_ptr_type : public type {
  public:

    scene_ptr_type(type_table *types) : type(types, "scene_ptr", "sp") { }
    virtual llvm::Type *llvm_type() const;

  };

};

#endif
