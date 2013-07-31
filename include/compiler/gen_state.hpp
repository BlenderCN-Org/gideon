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

    /*
      Used for tracking state in the current function.
      The target_sel value stores a code that cleanup blocks
      use to decide what to execute next. Possible values are:

      0 - Continue normally to the next block.
      1 - Jump to the next cleanup block until the top of the function is reached, then return.
      2 - Jump out to the next_iteration block of the current loop, cleaning up along the way (i.e. continue).
      3 - Jump through cleanup blocks until the loop has been exited (i.e. break).
      4 - Exception handling.
    */
    struct function_state {
      llvm::Function *func;
      type_spec return_ty;
      llvm::Value *rt_val; //location to be used for the return value
      llvm::Value *target_sel; //used by cleanup blocks
      llvm::Value *exception; //location of any exception value
      bool entry_point;

      llvm::BasicBlock *entry_block, *cleanup_block, *return_block, *except_block;
      
      llvm::MDNode *func_metadata;

      function_state(llvm::Function *func,
		     const type_spec &t,
		     bool entry_point,
		     llvm::Module *module, llvm::IRBuilder<> &builder);
    };
    
    /*
      Blocks relevant to a new scope created inside a function.
      
      body_block - Main body of this scope.
      exit_block - Cleanup code for the containing scope (in case this one needs to return/break/continue).
      next_block - Where to go if we leave this scope normally.
      metadata - Metadata to identify this scope for debugging purposes.
    */
    struct scope_state {
      llvm::BasicBlock *body_block, *exit_block, *next_block;
      
      llvm::MDNode *metadata;
    };
    
    /* 
       Extra information needed by loops to support break/continue.

       update_block - Block that updates any loop variables. The eventual destination of a 'continue' statement.
       top_scope - Depth of the scope at the top of this loop. Up until this level, break/continue should be branching into cleanup blocks only.
     */
    struct loop_blocks {
      llvm::BasicBlock *update_block;
      unsigned int top_scope;
    };

    typedef boost::function<void (llvm::Value*, llvm::Module*, llvm::IRBuilder<>&)> context_loader_type;

    struct class_context {
      llvm::Value *ctx;
      llvm::Type *ctx_type;
      context_loader_type loader;
    };

    std::vector<function_state> function_stack;
    
    std::vector<loop_blocks> loop_stack;
    std::vector<class_context> context_stack;

    //function control
    void push_function(const type_spec &t, bool entry_point, llvm::Function *f,
		       llvm::Module *module, llvm::IRBuilder<> &builder);
    void pop_function();

    llvm::Function *get_current_function();

    function_state &get_function_state() { return function_stack.back(); }
    llvm::BasicBlock *get_function_body();
    llvm::BasicBlock *get_cleanup_block();
    llvm::BasicBlock *get_return_block();
    llvm::Value *get_return_value_ptr();
    void set_jump_target(llvm::IRBuilder<> &builder, int code);
    
    //loop control

    void push_loop(llvm::BasicBlock *update);
    void pop_loop();
    bool inside_loop();
    llvm::BasicBlock *loop_update_block();
    bool at_loop_top();

    //distribution context control
    
    void push_context(llvm::Value *ctx, llvm::Type *ctx_type,
		      const boost::function<void (llvm::Value*, llvm::Module*, llvm::IRBuilder<>&)> &loader);
    void pop_context();

    bool has_context();
    void load_context(llvm::Module *module, llvm::IRBuilder<> &builder);

    void set_context(llvm::Value *ctx);
    llvm::Value *get_context();
    llvm::Type *get_context_type();

    //scope control
    void push_scope();
    void pop_scope();

    std::vector<scope_state> scope_state_stack;

    //Upon exiting a scope (via return/break/continue), where is the next cleanup block?
    llvm::BasicBlock *get_exit_block();

    //Next cleanup block in a nested scope, otherwise this returns the function's error handling block.
    llvm::BasicBlock *get_error_block();

    //The next block we go to when we exit a scope normally.
    llvm::BasicBlock *get_next_block();

    scope_state &get_scope_state() { return scope_state_stack.back(); }

    //debugging helpers

    llvm::MDNode *get_scope_metadata();
  };

};

#endif
