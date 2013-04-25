#include "compiler/ast/module.hpp"

using namespace raytrace;
using namespace llvm;
using namespace std;

codegen_void ast::module::codegen(Module *module, IRBuilder<> &builder) {
  state->variables.scope_push();
  state->functions.scope_push(name);

  //evaluate all internal declarations
  codegen_vector content_eval;
  for (auto it = content.begin(); it != content.end(); it++) {
    codegen_value eval = (*it)->codegen(module, builder);
    content_eval = errors::codegen_vector_push_back(content_eval, eval);
  }

  codegen_void fpop = state->functions.scope_pop(module, builder);
  codegen_void vpop = state->functions.scope_pop(module, builder);

  typedef errors::argument_value_join<codegen_vector, codegen_void, codegen_void>::result_value_type arg_val_type;
  boost::function<codegen_void (arg_val_type &)> op = [] (arg_val_type &) -> codegen_void { return nullptr; };
  return errors::codegen_call_args(op, content_eval, fpop, vpop);
}
