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
    codegen_value literal<float>::codegen(Module *, IRBuilder<> &) { return ConstantFP::get(getGlobalContext(), APFloat(value)); }
    
    template<>
    codegen_value literal<int>::codegen(Module *, IRBuilder<> &) { return ConstantInt::get(getGlobalContext(), APInt(8*sizeof(int), value, true)); }

    template<>
    codegen_value literal<bool>::codegen(Module *, IRBuilder<> &) { return ConstantInt::get(getGlobalContext(), APInt(1, value, true)); }

    template<>
    codegen_value literal<string>::codegen(Module *, IRBuilder<> &builder) {
      Value *is_const = ConstantInt::get(getGlobalContext(), APInt(1, true, true));
      Value *str_ptr = builder.CreateGlobalStringPtr(value, "str_data");
      
      Value *str = builder.CreateInsertValue(UndefValue::get(state->types["string"]->llvm_type()),
					     is_const, ArrayRef<unsigned int>(0), "new_str");
      str = builder.CreateInsertValue(str, str_ptr, ArrayRef<unsigned int>(1));
      return str;
    }
    
  };
};
