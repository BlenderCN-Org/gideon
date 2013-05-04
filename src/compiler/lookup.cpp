#include "compiler/lookup.hpp"

using namespace raytrace;
using namespace llvm;
using namespace std;

module_scope::iterator module_scope::find(const key_type &name) { return module->modules.find(name); }
module_scope::iterator module_scope::begin() { return module->modules.begin(); }
module_scope::iterator module_scope::end() { return module->modules.end(); }

module_scope::entry_type &module_scope::extract_entry(iterator it) {
  return it->second;
}

void module_scope::set(const key_type &name, const entry_type &entry) {
  module->modules[name] = entry;
}

namespace raytrace {

  template<>
  std::string symbol_name<module_scope::entry_type>() { return "module"; }

};

/** Lookup Functions **/

typed_value_container value_select_field(Value *value, type_spec &type,
					 name_path::const_iterator path_start, name_path::const_iterator path_end,
					 bool get_reference,
					 Module *module, IRBuilder<> &builder) {
  if (path_start == path_end) return typed_value(value, type);

  typed_value_container field = (get_reference ?
				 type->access_field_ptr(*path_start, value, module, builder) :
				 type->access_field(*path_start, value, module, builder));
  return errors::codegen_call(field, [&path_start, &path_end, get_reference, module, &builder] (typed_value &val) -> typed_value_container {
      return value_select_field(val.get<0>().extract_value(), val.get<1>(), path_start+1, path_end, get_reference, module, builder);
    });
}

typed_value_container raytrace::variable_lookup(module_symbol_table &top,
					       variable_symbol_table &variables,
					       const name_path &path,
					       Module *module, IRBuilder<> &builder) {
  const string &name = path.front();

  //lookup the name in the variable table
  try {
    //if found, treat as field selection
    variable_symbol_table::entry_type &var = variables.get(name);
    return value_select_field(var.value, var.type, path.begin() + 1, path.end(), false, module, builder);
  }
  catch (compile_error &e) {
    //ignore and move on
  }

  //check each module going up the stack
  for (auto module_it = top.scope_begin(); module_it != top.scope_end(); ++module_it) {
    module_object &curr_module = *module_it->get_module();

    //check any global variables
    auto var_it = curr_module.variables.find(name);
    if (var_it != curr_module.variables.end()) {
      //found global variable with correct name
      variable_symbol_table::entry_type &var = var_it->second;
      return value_select_field(var.value, var.type, path.begin() + 1, path.end(), false, module, builder);
    }

    //check any module names
  }

  stringstream err_ss;
  err_ss << "Could not find variable or module named '" << name << "'";
  return compile_error(err_ss.str());
}


typecheck_value value_select_field_type(type_spec &type,
					name_path::const_iterator path_start, name_path::const_iterator path_end) {
  if (path_start == path_end) return type;

  typecheck_value field_type = type->field_type(*path_start);
  
  return errors::codegen_call(field_type, [&path_start, &path_end] (type_spec &type) -> typecheck_value {
      return value_select_field_type(type, path_start+1, path_end);
    });
}

typecheck_value raytrace::variable_type_lookup(module_symbol_table &top,
					       variable_symbol_table &variables,
					       const name_path &path) {
  const string &name = path.front();
  
  //lookup the name in the variable table
  try {
    //if found, treat as field selection
    variable_symbol_table::entry_type &var = variables.get(name);
    return value_select_field_type(var.type, path.begin() + 1, path.end());
  }
  catch (compile_error &e) {
    //ignore and move on
  }

  //check each module going up the stack
  for (auto module_it = top.scope_begin(); module_it != top.scope_end(); ++module_it) {
    module_object &curr_module = *module_it->get_module();

    //check any global variables
    auto var_it = curr_module.variables.find(name);
    if (var_it != curr_module.variables.end()) {
      //found global variable with correct name
      variable_symbol_table::entry_type &var = var_it->second;
      return value_select_field_type(var.type, path.begin() + 1, path.end());
    }

    //check any module names
  }

  stringstream err_ss;
  err_ss << "Could not find variable or module named '" << name << "'";
  return compile_error(err_ss.str());
}

