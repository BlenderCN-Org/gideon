#include "compiler/gen_state.hpp"

using namespace raytrace;
using namespace llvm;
using namespace std;

void control_state::push_function_rt(const type_spec &t) {
  return_type_stack.push_back(t);
  function_scope_depth_stack.push_back(scope_depth());
}
void control_state::pop_function_rt() {
  function_scope_depth_stack.pop_back();
  return_type_stack.pop_back();
}

unsigned int control_state::function_start_depth() {
  return function_scope_depth_stack.back();
}

type_spec &control_state::return_type() { return return_type_stack.back(); }
    
void control_state::push_loop(BasicBlock *post, BasicBlock *next) {
  loop_blocks loop{post, next, scope_depth()};
  loop_stack.push_back(loop);
}

void control_state::pop_loop() { loop_stack.pop_back(); }
bool control_state::inside_loop() { return loop_stack.size() > 0; }

unsigned int control_state::loop_top_scope() { return loop_stack.back().top_scope; }
BasicBlock *control_state::post_loop() { return loop_stack.back().post_loop; }
BasicBlock *control_state::next_iter() { return loop_stack.back().post_loop; }

void control_state::push_scope() { scope_stack.push_back(1); }
void control_state::pop_scope() { scope_stack.pop_back(); }
bool control_state::scope_reaches_end() { return (scope_stack.back() != 0); }
void control_state::set_scope_reaches_end(bool b) { scope_stack.back() = (b ? 1 : 0); }
unsigned int control_state::scope_depth() { return scope_stack.size(); }

void control_state::push_context(Value *ctx, Type *ctx_type, const boost::function<void (Value*, Module*, IRBuilder<>&)> &loader) {
  class_context cc{ctx, ctx_type, loader};
  context_stack.push_back(cc);
}

void control_state::pop_context() { context_stack.pop_back(); }

bool control_state::has_context() { return (context_stack.size() > 0); }

void control_state::load_context(Module *module, IRBuilder<> &builder) {
  class_context &cc = context_stack.back();
  cc.loader(cc.ctx, module, builder);
}

Value *control_state::get_context() { return context_stack.back().ctx; }
void control_state::set_context(Value *ctx) { context_stack.back().ctx = ctx; }
Type *control_state::get_context_type() { return context_stack.back().ctx_type; }
