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

#include "compiler/ast/literal.hpp"

using namespace std;
using namespace llvm;

namespace raytrace {
  
  namespace ast {
    
    template<>
    type_spec get_literal_type<float>(parser_state *st) { return st->types["float"]; }

    template<>
    type_spec get_literal_type<int>(parser_state *st) { return st->types["int"]; }

    template<>
    type_spec get_literal_type<bool>(parser_state *st) { return st->types["bool"]; }

    template<>
    type_spec get_literal_type<string>(parser_state *st) { return st->types["string"]; }


    template<>
    typed_value_container literal<float>::codegen(Module *, IRBuilder<> &) { return typed_value(ConstantFP::get(getGlobalContext(), APFloat(value)),
												     get_literal_type<float>(state)); }
    template<>
    codegen_constant literal<float>::codegen_const_eval(Module *, IRBuilder<> &) {
      return typed_constant(ConstantFP::get(getGlobalContext(), APFloat(value)),
			    get_literal_type<float>(state));
    }
    
    template<>
    typed_value_container literal<int>::codegen(Module *, IRBuilder<> &) { return typed_value(ConstantInt::get(getGlobalContext(), APInt(8*sizeof(int), value, true)),
												   get_literal_type<int>(state)); }

    template<>
    codegen_constant literal<int>::codegen_const_eval(Module *, IRBuilder<> &) {
      return typed_constant(ConstantInt::get(getGlobalContext(), APInt(8*sizeof(int), value, true)),
			    get_literal_type<int>(state));
    }

    template<>
    typed_value_container literal<bool>::codegen(Module *, IRBuilder<> &) { return typed_value(ConstantInt::get(getGlobalContext(), APInt(1, value, true)),
												    get_literal_type<bool>(state)); }

    template<>
    codegen_constant literal<bool>::codegen_const_eval(Module *, IRBuilder<> &) {
      return typed_constant(ConstantInt::get(getGlobalContext(), APInt(1, value, true)),
			    get_literal_type<bool>(state));
    }

    template<>
    typed_value_container literal<string>::codegen(Module *, IRBuilder<> &builder) {
      Value *is_const = ConstantInt::get(getGlobalContext(), APInt(1, true, true));
      Value *str_ptr = builder.CreateGlobalStringPtr(value, "str_data");
      
      Value *str = builder.CreateInsertValue(UndefValue::get(state->types["string"]->llvm_type()),
					     is_const, ArrayRef<unsigned int>(0), "new_str");
      str = builder.CreateInsertValue(str, str_ptr, ArrayRef<unsigned int>(1));
      return typed_value(str, state->types["string"]);
    }

    template<>
    codegen_constant literal<string>::codegen_const_eval(Module *, IRBuilder<> &) {
      return errors::make_error<errors::error_message>("String literals may not be used as constant expressions", line_no, column_no);
    }
    
  };
};
