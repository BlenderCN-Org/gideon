#ifndef RT_AST_LITERAL_HPP
#define RT_AST_LITERAL_HPP

#include "compiler/ast/expression.hpp"

namespace raytrace {

  namespace ast {
    
    template<typename T>
    type_spec get_literal_type(parser_state *st);

    //Template class for literals.
    template<typename T>
    class literal : public expression {
    public:
      
      literal(parser_state *st, const T& v,
	      unsigned int line_no, unsigned int column_no) : expression(st, get_literal_type<T>(st),
									 line_no, column_no), value(v) {}
      virtual ~literal() { }
      
      virtual typed_value_container codegen(llvm::Module *module, llvm::IRBuilder<> &builder);
      virtual typecheck_value typecheck() { return { get_literal_type<T>(state) }; }
      virtual codegen_constant codegen_const_eval(llvm::Module *module, llvm::IRBuilder<> &builder);
      
    private:
      
      T value;
      
    };

  };

};

#endif
