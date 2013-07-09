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

#ifndef RT_COMPILER_CODEGEN_STATE_HPP
#define RT_COMPILER_CODEGEN_STATE_HPP

#include "compiler/types.hpp"

namespace raytrace {

  struct control_state {
    struct loop_blocks {
      llvm::BasicBlock *post_loop, *next_iter;
      unsigned int top_scope;
    };

    typedef boost::function<void (llvm::Value*, llvm::Module*, llvm::IRBuilder<>&)> context_loader_type;

    struct class_context {
      llvm::Value *ctx;
      llvm::Type *ctx_type;
      context_loader_type loader;
    };

    std::vector<type_spec> return_type_stack;
    std::vector<unsigned int> function_scope_depth_stack;

    std::vector<loop_blocks> loop_stack;
    std::vector<class_context> context_stack;
    std::vector<int> scope_stack;
    
    void push_function_rt(const type_spec &t);
    void pop_function_rt();
    type_spec &return_type();
    unsigned int function_start_depth();
    
    void push_loop(llvm::BasicBlock *post, llvm::BasicBlock *next);    
    void pop_loop();
    bool inside_loop();

    unsigned int loop_top_scope();
    llvm::BasicBlock *post_loop();
    llvm::BasicBlock *next_iter();

    //Used to track the presence of terminator statements in the current scope
    void push_scope();
    void pop_scope();
    bool scope_reaches_end();
    void set_scope_reaches_end(bool b);
    unsigned int scope_depth();

    void push_context(llvm::Value *ctx, llvm::Type *ctx_type,
		      const boost::function<void (llvm::Value*, llvm::Module*, llvm::IRBuilder<>&)> &loader);
    void pop_context();

    bool has_context();
    void load_context(llvm::Module *module, llvm::IRBuilder<> &builder);

    void set_context(llvm::Value *ctx);
    llvm::Value *get_context();
    llvm::Type *get_context_type();
  };

};

#endif
