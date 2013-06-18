#include "compiler/symboltable.hpp"
#include "compiler/type_conversion.hpp"

#include <stdexcept>

using namespace std;
using namespace raytrace;
using namespace llvm;

namespace raytrace {

  /* Variable Symbol Table */
  template<>
  string symbol_name<variable_entry>() { return "variable"; }
  
  template<>
  codegen_void destroy_entry<variable_entry>(variable_entry &entry, Module *module, IRBuilder<> &builder) {
    if (entry.destroy_on_scope_exit)
      return entry.type->destroy(entry.value, module, builder);

    return empty_type();
  }

  variable_scope::iterator variable_scope::find(const key_type &name) { return table.find(name); }
  variable_scope::iterator variable_scope::begin() { return table.begin(); }
  variable_scope::iterator variable_scope::end() { return table.end(); }

  variable_scope::entry_type &variable_scope::extract_entry(iterator it) {
    return it->second;
  }

  void variable_scope::set(const key_type &name, const entry_type &entry) {
    table[name] = entry;
  }

  codegen_void variable_scope::destroy(Module *module, IRBuilder<> &builder) {
    TerminatorInst *term = builder.GetInsertBlock()->getTerminator();
    if (term) return empty_type(); //we've already exited this block, don't add anymore code

    codegen_void rt = empty_type();

    for (auto it = begin(); it != end(); it++) {
      codegen_void d = destroy_entry<entry_type>(it->second, module, builder);
      rt = errors::merge_void_values(rt, d);
    }
    
    return rt;
  }


  /* Overloaded Functions */
  
  bool function_argument::operator==(const function_argument &rhs) const {
    if (type != rhs.type) return false;
    if (output != rhs.output) return false;
    return true;
  }

  string function_generate_name(const string &base_name, const string &scope_name, const vector<function_argument> &args) {
    stringstream ss;
    ss << "gdi." << scope_name << "." << args.size() << "." << base_name;
    
    for (auto it = args.begin(); it != args.end(); it++) {
      ss << "." << it->type->type_id;
    }
    
    return ss.str();
  }

  bool function_entry::operator==(const function_entry &rhs) const {
    if (return_type != rhs.return_type) return false;
    if (arguments.size() != rhs.arguments.size()) return false;
    for (auto l_it = arguments.begin(), r_it = rhs.arguments.begin(); l_it != arguments.end(); l_it++, r_it++) {
      if (*l_it->type != *r_it->type) return false; //compare by type_spec only
    }

    return true;
  }
  
  function_key function_entry::to_key() const {
    function_key key;
    key.name = name;
    for (auto it = arguments.begin(); it != arguments.end(); it++) key.arguments.push_back(it->type);
    return key;
  }
  
  function_entry function_entry::make_entry(const string &name, const string &scope_name,
					    const type_spec &return_type, const vector<function_argument> &arguments) {
    function_entry fe;
    fe.func = NULL;
    fe.name = name;
    fe.full_name = function_generate_name(name, scope_name, arguments);

    fe.external = false;
    fe.member_function = false;
    fe.return_type = return_type;
    fe.arguments = arguments;
    return fe;
  }
  
  function_overload_set::function_overload_set() { }
  
  void function_overload_set::insert(const function_entry &entry) {
    iterator it = versions.find(entry.full_name);
    if (it != versions.end()) throw runtime_error("Equivalent function already exists");
    
    versions[entry.full_name] = entry;
  }

  function_overload_set::iterator function_overload_set::find(const function_key &key) {
    for (auto it = versions.begin(); it != versions.end(); ++it) {
      function_entry &entry = it->second;
      bool equivalent = (key.arguments.size() == entry.arguments.size());
      if (equivalent) {
	for (unsigned int idx = 0; idx < entry.arguments.size(); ++idx) {
	  const function_argument &arg = entry.arguments[idx];
	  const type_spec &param_ts = key.arguments[idx];
	  
	  if (*arg.type != *param_ts) {
	    equivalent = false;
	    break;
	  }
	}
      }

      if (equivalent) return it;
    }

    return end();
  }
  
  function_overload_set::iterator function_overload_set::find_best(const function_key &key,
								   const type_conversion_table &conversions) {
    vector<iterator> candidates;
    for (auto it = versions.begin(); it != versions.end(); it++) {
      if (is_viable(key, it->second, conversions)) candidates.push_back(it);
    }
    
    if (candidates.size() == 0) return end();

    //select the best candidate
    int min_score = numeric_limits<int>::max();
    int score_count = 0;
    iterator best_it = end();

    for (auto it = candidates.begin(); it != candidates.end(); it++) {
      int score = candidate_score(key, (*it)->second, conversions);
      if (score < min_score) {
	min_score = score;
	score_count = 1;
	best_it = *it;
      }
      else if (score == min_score) score_count++;
    }

    if (score_count > 1) throw runtime_error("Function call is ambiguous");

    return best_it;
  }

  function_overload_set::iterator function_overload_set::begin() { return versions.begin(); }
  function_overload_set::iterator function_overload_set::end() { return versions.end(); }

  bool function_overload_set::is_viable(const function_key &key, const function_entry &entry,
					const type_conversion_table &conversions) {
    int unused;
    
    if (entry.arguments.size() != key.arguments.size()) return false;
    for (unsigned int idx = 0; idx < entry.arguments.size(); idx++) {
      const function_argument &arg = entry.arguments[idx];
      const type_spec &param_ts = key.arguments[idx];

      if (!conversions.can_convert(param_ts, arg.type, unused, unused)) return false;
    }
    
    return true;
  }

  int function_overload_set::candidate_score(const function_key &key, const function_entry &entry,
					     const type_conversion_table &conversions) {
    int unused;
    int score = 0;

    for (unsigned int idx = 0; idx < entry.arguments.size(); idx++) {
      const function_argument &arg = entry.arguments[idx];
      const type_spec &param_ts = key.arguments[idx];
      int cast_cost = 0;

      if (arg.output && (*param_ts != *arg.type)) return numeric_limits<int>::max();
      if (!conversions.can_convert(param_ts, arg.type, cast_cost, unused)) return numeric_limits<int>::max();
      score += cast_cost;
    }
    
    return score;
  }

  template<>
  string symbol_name<function_entry>() { return "function"; }

  template<>
  string symbol_name<function_key>() { return "function"; }

  function_scope::function_scope(const string &name) :
    name(name)
  {
    
  }

  function_scope::iterator &function_scope::iterator::operator++() {
    version_it++;
    if (version_it == name_it->second.end()) {
      name_it++;
      if (name_it != end) version_it = name_it->second.begin();
    }

    return *this;
  }

  function_entry &function_scope::iterator::operator*() {
    return version_it->second;
  }
  
  bool function_scope::iterator::operator==(const iterator &rhs) const {
    if (name_it != rhs.name_it) return false;
    if (name_it == end) return true; //don't compare versions if the top-level iterator is at the end

    if (version_it != rhs.version_it) return false;
    return true;
  }

  function_scope::iterator function_scope::begin() {
    table_type::iterator start = table.begin();
    iterator it;
    it.name_it = start;
    it.end = table.end();
    if (start != it.end) it.version_it = start->second.begin();
    
    return it;
  }

  function_scope::iterator function_scope::end() {
    iterator it;
    it.name_it = table.end();
    it.end = table.end();
    return it;
  }

  function_scope::entry_type &function_scope::extract_entry(iterator it) {
    return *it;
  }

  void function_scope::set(const key_type &f, const entry_type &entry) {
    table[f.name].insert(entry);
  }

  function_scope::iterator function_scope::find(const key_type &f) {
    iterator result = end();
    table_type::iterator name_it = table.find(f.name);
    if (name_it == table.end()) return end();
    function_overload_set::iterator version = name_it->second.find(f);
    if (version == name_it->second.end()) return end();
    
    result.name_it = name_it;
    result.end = table.end();
    result.version_it = version;

    return result;
  }

  function_scope::iterator function_scope::find_best(const key_type &f,
						     const type_conversion_table &conversions) {
    iterator result = end();
    table_type::iterator name_it = table.find(f.name);
    if (name_it == table.end()) return end();
    function_overload_set::iterator version = name_it->second.find_best(f, conversions);
    if (version == name_it->second.end()) return end();
    
    result.name_it = name_it;
    result.end = table.end();
    result.version_it = version;

    return result;
  }
  
};

bool raytrace::function_entry::compare(const type_spec &rt, const vector<type_spec> &args) {
  if (return_type != rt) return false;
  if (arguments.size() != args.size()) return false;
  
  for (unsigned int i = 0; i < arguments.size(); i++) {
    if (arguments[i].type != args[i]) return false;
  }
  
  return true;
}

/* Type Symbol Table */

namespace raytrace {
  template<>
  string symbol_name<type*>() { return "type"; }
};

type_scope::iterator type_scope::find(const key_type &name) { return table.find(name); }
type_scope::iterator type_scope::begin() { return table.begin(); }
type_scope::iterator type_scope::end() { return table.end(); }

type_scope::entry_type &type_scope::extract_entry(iterator it) { return it->second; }
    
void type_scope::set(const key_type &name, const entry_type &entry) {
  table[name] = entry;
}
