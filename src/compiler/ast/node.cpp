#include "compiler/ast/node.hpp"

using namespace std;
using namespace raytrace;
using namespace llvm;

void ast::ast_node::push_scope(const string &name) {
  variables().scope_push(name);
  functions().scope_push(name);
  state->control.push_scope();
}

void ast::ast_node::pop_scope(Module *module, IRBuilder<> &builder) {
  bool reaches_end = state->control.scope_reaches_end();

  state->control.pop_scope();
  functions().scope_pop(module, builder, reaches_end);
  variables().scope_pop(module, builder, reaches_end);
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

void ast::ast_node::push_module(const string &name, bool exported) {
  state->modules.scope_push(name);
  if (exported) state->exports.push_module(name);
}
 
codegen_void ast::ast_node::pop_module(const string &name, bool exported,
				       Module *module, IRBuilder<> &builder) {
  void_vector result;

  //save this module
  auto scope_it = state->modules.scope_begin();
  auto parent_scope = scope_it + 1;
  if (parent_scope != state->modules.scope_end()) {
    codegen_void saved = empty_type();
    module_ptr &module = scope_it->get_module();
    auto mod_it = parent_scope->get_module()->modules.find(name);
    if (mod_it != parent_scope->get_module()->modules.end()) {
      stringstream err_ss;
      err_ss << "Redeclaration of module '" << name << "'";
      saved = errors::make_error<errors::error_message>(err_ss.str(), line_no, column_no);
    }
    else parent_scope->get_module()->modules[name] = module;

    result = errors::codegen_vector_push_back(result, saved);
  }

  if (exported) state->exports.pop_module();
  state->modules.scope_pop(module, builder, false);

  return errors::codegen_call<void_vector, codegen_void>(result, [] (vector<empty_type> &arg) -> codegen_void {
      return empty_type();
    });
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
    variables().scope_destroy(N, module, builder);
    functions().scope_destroy(N, module, builder);
  }
}

void ast::ast_node::exit_to_loop_scope(Module *module, IRBuilder<> &builder) {
  assert(state->control.inside_loop());
  
  unsigned int end_scope = state->control.loop_top_scope();
  if (state->control.scope_depth() > end_scope) {
    unsigned int N = state->control.scope_depth() - end_scope;
    variables().scope_destroy(N, module, builder);
    functions().scope_destroy(N, module, builder);
  }
}

void ast::ast_node::exit_function(Module *module, IRBuilder<> &builder) {
  unsigned int end_scope = state->control.function_start_depth();
  if (state->control.scope_depth() > end_scope) {
    unsigned int N = state->control.scope_depth() - end_scope;
    variables().scope_destroy(N, module, builder);
    functions().scope_destroy(N, module, builder);
  }
}

variable_symbol_table &ast::ast_node::variables() {
  return state->variables;
}

variable_scope &ast::ast_node::global_variables() {
  return state->modules.scope().get_module()->variables;
}

function_scope &ast::ast_node::function_table() {
  if (state->functions.depth() > 0) return state->functions.scope();
  assert(state->modules.depth() > 0); //this shouldn't be called if there is nowhere to save a function
  return state->modules.scope().get_module()->functions;
}

string ast::ast_node::function_scope_name() {
  assert(state->modules.depth() > 0);

  stringstream ss;
  ss << state->modules.scope_name();
  if (state->functions.depth() > 0) ss << "_" << state->functions.scope_name();
  
  return ss.str();
}

function_symbol_table &ast::ast_node::functions() {
  return state->functions;
}

const function_symbol_table &ast::ast_node::functions() const {
  return state->functions;
}

typecheck_value ast::ast_node::variable_type_lookup(const string &name) {
  //lookup the name in the local variable table
  try {
    variable_symbol_table::entry_type &var = variables().get(name);
    return var.type;
  }
  catch (compile_error &e) { } //ignore and move on 

  //check globals and module names
  for (auto scope_it = state->modules.scope_begin(); scope_it != state->modules.scope_end(); ++scope_it) {
    module_ptr &module = scope_it->get_module();
    
    //look for a global variable name
    auto var_it = module->variables.find(name);
    if (var_it != module->variables.end()) {
      variable_symbol_table::entry_type &entry = var_it->second;
      return entry.type;
    }

    //check for a module name
    auto mod_it = module->modules.find(name);
    if (mod_it != module->modules.end()) {
      return state->types["module"];
    }
  }

  stringstream err_ss;
  err_ss << "No such variable or module named '" << name << "'";

  return errors::make_error<errors::error_message>(err_ss.str(), line_no, column_no);  
}

typed_value_container ast::ast_node::variable_lookup(const string &name) {
  //lookup the name in the local variable table
  try {
    variable_symbol_table::entry_type &var = variables().get(name);
    return typed_value(var.value, var.type);
  }
  catch (compile_error &e) { } //ignore and move on 

  //check globals and module names
  for (auto scope_it = state->modules.scope_begin(); scope_it != state->modules.scope_end(); ++scope_it) {
    module_ptr &module = scope_it->get_module();
    
    //look for a global variable name
    auto var_it = module->variables.find(name);
    if (var_it != module->variables.end()) {
      variable_symbol_table::entry_type &entry = var_it->second;
      return typed_value(entry.value, entry.type);
    }

    //check for a module name
    auto mod_it = module->modules.find(name);
    if (mod_it != module->modules.end()) {
      return typed_value(mod_it->second, state->types["module"]);
    }
  }

  stringstream err_ss;
  err_ss << "No such variable or module named '" << name << "'";
  return errors::make_error<errors::error_message>(err_ss.str(), line_no, column_no);
}

ast::ast_node::entry_or_error ast::ast_node::function_lookup(const function_key &fkey) {
  try {
    //search each scope in this module (starting at the closest, working backwards)
    for (auto fscope = functions().scope_begin(); fscope != functions().scope_end(); ++fscope) {
      auto f_it = fscope->find_best(fkey, state->type_conversions);
      if (f_it != fscope->end()) return &(*f_it);
    }
    
    //look up the module stack
    for (auto mod_scope = state->modules.scope_begin(); mod_scope != state->modules.scope_end(); ++mod_scope) {
      function_scope &fscope = mod_scope->get_module()->functions;
      auto f_it = fscope.find_best(fkey, state->type_conversions);
      if (f_it != fscope.end()) return &(*f_it);
    }
  }
  catch (runtime_error &e) {
    //ambiguous function call case
    return errors::make_error<errors::error_message>(e.what(), line_no, column_no);
  }

  stringstream err_ss;
  err_ss << "Undeclared function '" << fkey.name << "'";
  return errors::make_error<errors::error_message>(err_ss.str(), line_no, column_no);
}

code_value ast::ast_node::typecast(typed_value_container &src, const type_spec &dst_type,
				   bool make_copy, bool destroy_on_convert,
				   Module *module, IRBuilder<> &builder) {
  boost::function<code_value (typed_value &)> cast_op = [this, &dst_type,
							 make_copy, destroy_on_convert,
							 module, &builder] (typed_value &args) -> code_value {
    type_spec src_type = args.get<1>();
    
    Value *src = args.get<0>().extract_value();
    if (make_copy) src = src_type->copy(src, module, builder);
    
    code_value result = state->type_conversions.convert(src_type, src, dst_type, module, builder);
    if ((*src_type != *dst_type) && destroy_on_convert) src_type->destroy(src, module, builder);

    return result;
  };

  return errors::codegen_call<typed_value_container, code_value>(src, cast_op);
}

code_value ast::ast_node::typecast(Value *src, 
				   const type_spec &src_type, const type_spec &dst_type,
				   bool make_copy, bool destroy_on_convert,
				   Module *module, IRBuilder<> &builder) {
  if (make_copy) src = src_type->copy(src, module, builder);    
  code_value result = state->type_conversions.convert(src_type, src, dst_type, module, builder);
  
  if ((*src_type != *dst_type) && destroy_on_convert) src_type->destroy(src, module, builder);
  return result;
}

typecheck_value ast::ast_node::typename_lookup(const string &name) {
  //lookup the name in the local variable table
  try {
    return state->user_types.get(name);
  }
  catch (compile_error &e) { } //ignore and move on 

  //check defined in the modules
  for (auto scope_it = state->modules.scope_begin(); scope_it != state->modules.scope_end(); ++scope_it) {
    module_ptr &module = scope_it->get_module();

    auto type_it = module->types.find(name);
    if (type_it != module->types.end()) {
      return type_it->second;
    }
  }

  //check built-in types
  if (state->types.has_type(name)) return state->types[name];

  stringstream err_ss;
  err_ss << "No such type named '" << name << "'";
  return errors::make_error<errors::error_message>(err_ss.str(), line_no, column_no);
}
