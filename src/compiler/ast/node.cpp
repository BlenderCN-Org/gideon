#include "compiler/ast/node.hpp"

using namespace std;
using namespace raytrace;
using namespace llvm;

void ast::ast_node::push_scope(const string &name) {
  state->variables.scope_push(name);
  state->functions.scope_push(name);
  state->control.push_scope();
  cout << "Scope Depth: " << state->control.scope_depth() << endl;
}

void ast::ast_node::pop_scope(Module *module, IRBuilder<> &builder) {
  bool reaches_end = state->control.scope_reaches_end();

  state->control.pop_scope();
  state->functions.scope_pop(module, builder, reaches_end);
  state->variables.scope_pop(module, builder, reaches_end);
  cout << "Scope Depth: " << state->control.scope_depth() << endl;
}

void ast::ast_node::push_function(const type_spec &t) {
  //order is important here, we need to know which scope we can exit to
  state->control.push_function_rt(t);
  push_scope();
}
  
void ast::ast_node::pop_function(Module *module, IRBuilder<> &builder) {
  pop_scope(module, builder);
  state->control.pop_function_rt();
}

void ast::ast_node::push_distribution_context(const string &name, Type *param_ptr_type, const control_state::context_loader_type &loader) {
  state->control.push_context(NULL, param_ptr_type, loader);
  push_scope(name);
}

void ast::ast_node::pop_distribution_context(Module *module, IRBuilder<> &builder) {
  pop_scope(module, builder);
  state->control.pop_context();
}

void ast::ast_node::exit_loop_scopes(Module *module, IRBuilder<> &builder) {
  assert(state->control.inside_loop());
  
  unsigned int end_scope = state->control.loop_top_scope();
  if (state->control.scope_depth() > end_scope) {
    unsigned int N = state->control.scope_depth() - (end_scope - 1);
    cout << "N: " << N << endl;

    state->variables.scope_destroy(N, module, builder);
    state->functions.scope_destroy(N, module, builder);
  }
}

void ast::ast_node::exit_to_loop_scope(Module *module, IRBuilder<> &builder) {
  assert(state->control.inside_loop());
  
  unsigned int end_scope = state->control.loop_top_scope();
  if (state->control.scope_depth() > end_scope) {
    unsigned int N = state->control.scope_depth() - end_scope;
    cout << "N: " << N << endl;
    
    state->variables.scope_destroy(N, module, builder);
    state->functions.scope_destroy(N, module, builder);
  }
}

void ast::ast_node::exit_function(Module *module, IRBuilder<> &builder) {
  unsigned int end_scope = state->control.function_start_depth();
  if (state->control.scope_depth() > end_scope) {
    unsigned int N = state->control.scope_depth() - end_scope;
    cout << "N: " << N << endl;
    
    state->variables.scope_destroy(N, module, builder);
    state->functions.scope_destroy(N, module, builder);
  }
}
