#ifndef RT_COMPILER_CODEGEN_STATE_HPP
#define RT_COMPILER_CODEGEN_STATE_HPP

#include "compiler/types.hpp"

namespace raytrace {

  struct control_state {
    struct loop_blocks {
      llvm::BasicBlock *post_loop, *next_iter;
    };

    std::vector<type_spec> return_type_stack;
    std::vector<loop_blocks> loop_stack;
    
    void push_function_rt(const type_spec &t) { return_type_stack.push_back(t); }
    void pop_function_rt() { return_type_stack.pop_back(); }
    type_spec &return_type() { return return_type_stack.back(); }
    
    void push_loop(llvm::BasicBlock *post, llvm::BasicBlock *next) {
      loop_blocks loop{post, next};
      loop_stack.push_back(loop);
    }
    
    void pop_loop() { loop_stack.pop_back(); }
    bool inside_loop() { return loop_stack.size() > 0; }

    llvm::BasicBlock *post_loop() { return loop_stack.back().post_loop; }
    llvm::BasicBlock *next_iter() { return loop_stack.back().post_loop; }
  };

};

#endif
