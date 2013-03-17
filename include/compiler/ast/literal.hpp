#ifndef RT_AST_LITERAL_HPP
#define RT_AST_LITERAL_HPP

#include "compiler/ast/expression.hpp"

namespace raytrace {

  namespace ast {
    
    //Template class for literals.
    template<typename T>
    class literal : public expression {
    public:
      
      literal(const T& v) : expression(get_type_code<T>()), value(v) {}
      virtual ~literal() { }
      virtual llvm::Value *codegen(llvm::Module *module, llvm::IRBuilder<> &builder);
      virtual type_spec typecheck() { return { get_type_code<T>() }; }
      
    private:
      
      T value;
      
    };

  };

};

#endif
