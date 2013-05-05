#include "compiler/ast/module.hpp"
#include "compiler/ast/expression.hpp"
#include "compiler/ast/alias.hpp"

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
  
  codegen_void save_module = pop_module(name, module, builder);
  
  typedef errors::argument_value_join<codegen_vector, codegen_void>::result_value_type arg_val_type;
  boost::function<codegen_value (arg_val_type &)> op = [] (arg_val_type &) -> codegen_void { return nullptr; };
  return errors::codegen_call_args(op, content_eval, save_module);
}

/* Import Declaration */

ast::import_declaration::import_declaration(parser_state *st, const expression_ptr &module_path,
					    unsigned int line_no, unsigned int column_no) :
  global_declaration(st),
  module_path(module_path)
{
  
}

codegen_value ast::import_declaration::codegen(Module *module, IRBuilder<> &builder) {
  code_value m = module_path->codegen_module();
  auto import_func = [this, module, &builder] (value &val) -> codegen_value {
    module_ptr m = val.extract_module();

    //copy over all symbols (by generating alias nodes).
    codegen_vector alias_results;
    
    //copy variables
    for (auto var_it = m->variables.begin(); var_it != m->variables.end(); ++var_it) {
      variable_scope::entry_type &entry = var_it->second;
      global_variable_alias alias(state, entry.value->getName(), entry.type,
				  var_it->first, 0, 0);

      codegen_value result = alias.codegen(module, builder);
      alias_results = errors::codegen_vector_push_back(alias_results, result);
    }

    //copy functions
    for (auto func_it = m->functions.begin(); func_it != m->functions.end(); ++func_it) {
      function_scope::entry_type &entry = *func_it;
      function_alias alias(state, entry, entry.name, 0, 0);

      codegen_value result = alias.codegen(module, builder);
      alias_results = errors::codegen_vector_push_back(alias_results, result);
    }
    
    //copy sub-modules
    for (auto mod_it = m->modules.begin(); mod_it != m->modules.end(); ++mod_it) {
      module_ptr &curr_m = state->modules.scope().get_module();
      auto name_it = curr_m->modules.find(mod_it->first);
      if (name_it != curr_m->modules.end()) {
	//module name already exists, return an error
	stringstream err_ss;
	err_ss << "Cannot import module named '" << mod_it->first << ".' A module with that name already exists.";
	codegen_value err_val = compile_error(err_ss.str());
	alias_results = errors::codegen_vector_push_back(alias_results, err_val);
      }
      
      export_module(mod_it->first, mod_it->second);
      curr_m->modules.insert(*mod_it);
    }

    return errors::codegen_call<codegen_vector, codegen_value>(alias_results, [] (vector<Value*> &) -> codegen_value {
	return nullptr;
      });
  };
  
  return errors::codegen_call<code_value, codegen_value>(m, import_func);
}

void ast::import_declaration::export_module(const string &name, module_ptr &m) {
  state->exports.push_module(name);

  //export all variables
  for (auto var_it = m->variables.begin(); var_it != m->variables.end(); ++var_it) {
    variable_scope::entry_type &entry = var_it->second;
    exports::variable_export exp;
    exp.name = var_it->first;
    exp.full_name = entry.value->getName();
    exp.type = entry.type;
    state->exports.add_variable(exp);
  }

  //export all functions
  for (auto func_it = m->functions.begin(); func_it != m->functions.end(); ++func_it) {
    function_scope::entry_type &entry = *func_it;
    exports::function_export exp;
    exp.name = entry.name;
    exp.full_name = entry.full_name;
    exp.return_type = entry.return_type;
    exp.arguments = entry.arguments;
    state->exports.add_function(exp);
  }

  //recursively export submodules
  for (auto mod_it = m->modules.begin(); mod_it != m->modules.end(); ++mod_it) {
    export_module(mod_it->first, mod_it->second);
  }

  state->exports.pop_module();
}
