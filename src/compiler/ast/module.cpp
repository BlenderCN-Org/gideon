#include "compiler/ast/module.hpp"
#include "compiler/ast/expression.hpp"
#include "compiler/ast/alias.hpp"
#include "compiler/rendermodule.hpp"

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
  boost::function<codegen_value (arg_val_type &)> op = [] (arg_val_type &) -> codegen_value { return nullptr; };
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
	codegen_value err_val = errors::make_error<errors::error_message>(err_ss.str(), line_no, column_no);
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

/* Load Declaration */

ast::load_declaration::load_declaration(parser_state *st, const string &source_name,
					unsigned int line_no, unsigned int column_no) :
  global_declaration(st),
  source_name(source_name),
  line_no(line_no), column_no(column_no)
{
  
}

codegen_value ast::load_declaration::codegen(Module *module, IRBuilder<> &builder) {
  if (is_loaded()) {
    return nullptr; //only load a module once
  }
  
  if (!has_export_table()) {
    stringstream err_ss;
    err_ss << "Unable to load file '" << source_name << "'";
    return errors::make_error<errors::error_message>(err_ss.str(), line_no, column_no);
  }

  export_table &exports = get_export_table();
  
  codegen_vector results;
  vector<global_declaration_ptr> subtree = generate_subtree(exports);

  for (auto it = subtree.begin(); it != subtree.end(); ++it) {
    codegen_value v = (*it)->codegen(module, builder);
    results = errors::codegen_vector_push_back(results, v);
  }

  set_loaded();
  
  return errors::codegen_call<codegen_vector, codegen_value>(results, [] (vector<Value*> &arg) -> codegen_value {
      return nullptr;
    });
}

bool ast::load_declaration::has_export_table() {
  return state->objects->has_object(source_name);
}

export_table &ast::load_declaration::get_export_table() {
  return state->objects->get_export_table(source_name);
}

bool ast::load_declaration::is_loaded() {
  return (state->object_is_loaded.find(source_name) != state->object_is_loaded.end());
}

void ast::load_declaration::set_loaded() {
  state->object_is_loaded.insert(source_name);
}

vector<ast::global_declaration_ptr> ast::load_declaration::generate_subtree(export_table &exports) {
  vector<global_declaration_ptr> imported;
  for (auto mod_it = exports.top_begin(); mod_it != exports.top_end(); ++mod_it) {
    imported.push_back(get_module_content(mod_it->second));
  }
  return imported;
}

ast::global_declaration_ptr ast::load_declaration::get_module_content(exports::module_export::pointer &m) {
  vector<global_declaration_ptr> module_content;
  
  //alias all variables
  for (auto var_it = m->variables.begin(); var_it != m->variables.end(); ++var_it) {
    module_content.push_back(ast::global_declaration_ptr(new ast::global_variable_alias(state, var_it->full_name, var_it->type,
											var_it->name, line_no, column_no)));
  }
  
  //alias all functions
  for (auto func_it = m->functions.begin(); func_it != m->functions.end(); ++func_it) {
    function_entry entry;
    entry.name = func_it->name;
    entry.full_name = func_it->full_name;
    entry.external = false;
    entry.member_function = false;
    entry.return_type = func_it->return_type;
    
    entry.arguments = func_it->arguments;
    module_content.push_back(ast::global_declaration_ptr(new ast::function_alias(state, entry, func_it->name,
										 line_no, column_no)));
  }
  
  //copy in all submodules
  for (auto mod_it = m->modules.begin(); mod_it != m->modules.end(); ++mod_it) {
    module_content.push_back(get_module_content(mod_it->second));
  }

  return ast::global_declaration_ptr(new ast::module(state, m->name, module_content,
						     line_no, column_no));
}
