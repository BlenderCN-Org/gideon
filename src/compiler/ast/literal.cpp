#include "compiler/ast/literal.hpp"

using namespace std;
using namespace llvm;

namespace raytrace {
  
  namespace ast {

    template<>
    Value *literal<float>::codegen(Module *, IRBuilder<> &) { return ConstantFP::get(getGlobalContext(), APFloat(value)); }
    
    template<>
    Value *literal<int>::codegen(Module *, IRBuilder<> &) { return ConstantInt::get(getGlobalContext(), APInt(8*sizeof(int), value, true)); }

    template<>
    Value *literal<bool>::codegen(Module *, IRBuilder<> &) { return ConstantInt::get(getGlobalContext(), APInt(1, value, true)); }
    
  };
};
