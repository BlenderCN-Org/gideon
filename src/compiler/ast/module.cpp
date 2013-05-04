#include "compiler/ast/module.hpp"

using namespace raytrace;
using namespace llvm;
using namespace std;

codegen_value ast::module::codegen(Module *module, IRBuilder<> &builder) {
  push_module(name);

  //evaluate all internal declarations
  codegen_vector content_eval;
  for (auto it = content.begin(); it != content.end(); it++) {
    codegen_value eval = (*it)->codegen(module, builder);
    content_eval = errors::codegen_vector_push_back(content_eval, eval);
  }

  /*function_scope &fscope = functions().scope();
  for (auto it = fscope.begin(); it != fscope.end(); ++it) {
    cout << "Function Name: " << (*it).name << endl;
  }

  variable_scope &vscope = variables().scope();
  for (auto it = vscope.begin(); it != vscope.end(); ++it) {
    cout << "Variable Name: " <<  it->first << endl;
    }*/

  pop_module(name, module, builder);
  
  typedef errors::argument_value_join<codegen_vector>::result_value_type arg_val_type;
  boost::function<codegen_value (arg_val_type &)> op = [] (arg_val_type &) -> codegen_void { return nullptr; };
  return errors::codegen_call_args(op, content_eval);
}
