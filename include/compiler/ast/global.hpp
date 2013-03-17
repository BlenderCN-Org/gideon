#ifndef RT_AST_GLOBAL_HPP
#define RT_AST_GLOBAL_HPP

namespace raytrace {

  namespace ast {

    /* Base class for declarations that may appear at the top-level of a shader file. */
    class global_declaration {
    public:

      global_declaration() {}
      virtual ~global_declaration() {}
      virtual llvm::Value *codegen(llvm::Module *module, llvm::IRBuilder<> &builder) = 0;
      
    };

    typedef std::shared_ptr<global_declaration> global_declaration_ptr;
    
  };

};

#endif
