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

/* Export Table */

bool exports::variable_export::operator==(const variable_export &lhs) const {
  return name == lhs.name;
}

size_t exports::hash_value(const variable_export &v) {
  boost::hash<std::string> hasher;
  return hasher(v.name);
}

string exports::function_export::get_hash_string() const {
  stringstream ss;
  ss << name << "." << return_type->type_id;
  for (auto it = arguments.begin(); it != arguments.end(); ++it) {
    ss << "." << it->type->type_id;
  }

  return ss.str();
}

bool exports::function_export::operator==(const function_export &lhs) const {
  return get_hash_string() == lhs.get_hash_string();
}

size_t exports::hash_value(const function_export &f) {
  boost::hash<std::string> hasher;
  return hasher(f.get_hash_string());
}

static void indent_dump(unsigned int N) {
  for (unsigned int i = 0; i < N; ++i) cout << " ";
}

void exports::module_export::dump(unsigned int indent) {
  indent_dump(indent);
  cout << "module " << name << " {" << endl;
  
  indent_dump(indent+2);
  cout << "variables {" << endl;
  for (auto var_it = variables.begin(); var_it != variables.end(); ++var_it) {
    indent_dump(indent + 4);
    cout << var_it->name << " | " << var_it->full_name << " | " << var_it->type->name << endl;
  }
  indent_dump(indent+2);
  cout << "}" << endl;

  indent_dump(indent+2);
  cout << "functions {" << endl;
  for (auto func_it = functions.begin(); func_it != functions.end(); ++func_it) {
    indent_dump(indent + 4);
    cout << func_it->get_hash_string() << " | " << func_it->full_name << endl;
  }
  indent_dump(indent+2);
  cout << "}" << endl;
  
  for (auto mod_it = modules.begin(); mod_it != modules.end(); ++mod_it) {
    mod_it->second->dump(indent + 2);
  }

  indent_dump(indent);
  cout << "}" << endl << endl;
}

codegen_void export_table::add_variable(const variable_export &variable) {
  if (module_stack.size() == 0) return empty_type();
  module_export::pointer &m = module_stack.back();
  m->variables.insert(variable);
  return empty_type();
}

codegen_void export_table::add_function(const function_export &function) {
  if (module_stack.size() == 0) return empty_type();
  module_export::pointer &m = module_stack.back();
  m->functions.insert(function);
  return empty_type();
}


void export_table::push_module(const string &name) {
  module_export::pointer m(new module_export);
  m->name = name;
  module_stack.push_back(m);
}

codegen_void export_table::pop_module() {
  assert(module_stack.size() > 0);

  module_export::pointer m = module_stack.back();
  module_stack.pop_back();

  boost::unordered_map<string, module_export::pointer> &table = (module_stack.size() > 0 ?
								 module_stack.back()->modules :
								 top_modules);
  auto mod_it = table.find(m->name);
  if (mod_it != table.end()) {
    stringstream err_ss;
    err_ss << "Redeclaration of module '" << m->name << "'";
    return errors::make_error<errors::error_message>(err_ss.str(), 0, 0);
  }

  table[m->name] = m;
  return empty_type();
}

string export_table::scope_name() {
  stringstream ss;
  for (auto scope_it = module_stack.begin(); scope_it != module_stack.end(); ++scope_it) {
    ss << "." << (*scope_it)->name;
  }
  return ss.str();
}

void export_table::dump() {
  for (auto mod_it = top_modules.begin(); mod_it != top_modules.end(); ++mod_it) mod_it->second->dump(0);
}

void export_table::foreach_function_type(function_export::export_type type,
					 /* inout */ boost::unordered_set<string> &names_seen,
					 const boost::function<void (const string &, const string &)> &on_function) const {
  vector<module_export::pointer> module_ptr_stack;
  vector<string> path_stack;

  for (auto mod_it = top_modules.begin(); mod_it != top_modules.end(); ++mod_it) {
    module_ptr_stack.push_back(mod_it->second);
    path_stack.push_back(mod_it->first);
  }

  while (module_ptr_stack.size() > 0) {
    module_export::pointer mod = module_ptr_stack.back();
    string path_name = path_stack.back();

    path_stack.pop_back();
    module_ptr_stack.pop_back();

    for (auto func_it = mod->functions.begin(); func_it != mod->functions.end(); ++func_it) {
      if (func_it->type == type) {
	bool already_seen = (names_seen.find(func_it->full_name) != names_seen.end());
	
	if (!already_seen) {
	  string func_path = path_name + string(".") + func_it->name;
	  on_function(func_path, func_it->full_name);
	  names_seen.insert(func_it->full_name);
	}
      }
    }

    for (auto mod_it = mod->modules.begin(); mod_it != mod->modules.end(); ++mod_it) {
      module_ptr_stack.push_back(mod_it->second);
      path_stack.push_back(path_name + string(".") + mod_it->first);
    }
  }
}
