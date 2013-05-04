#ifndef RT_SYMBOL_TABLE_HPP
#define RT_SYMBOL_TABLE_HPP

#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>

#include "compiler/types.hpp"

#include "llvm/DerivedTypes.h"
#include "llvm/Function.h"

#include <iostream>
#include <string>
#include <sstream>
#include <map>
#include <vector>

#include <stdexcept>

namespace raytrace {
  
  //Returns a string name that the symbol table uses for error generation.
  template<typename SymType>
  std::string symbol_name();
  
  /* A structure of nested symbol scopes. */
  template<typename Scope>
  class scoped_symbol_table {
  public:

    typedef Scope scope_type;
    typedef typename Scope::entry_type entry_type;
    typedef typename Scope::key_type key_type;
    typedef typename std::vector<Scope>::reverse_iterator scope_iterator;

    //scoped_symbol_table() { scope_push(""); }

    //Searches for the given key, throwing an exception if it does not exist.
    entry_type &get(const key_type &name) {
      unsigned int end_idx = table.size() - 1;
      for (unsigned int idx = 0; idx < table.size(); idx++) {
	scope_type &scope = table[end_idx - idx];
	
	typename scope_type::iterator it = scope.find(name);
	if (it != scope.end()) return scope.extract_entry(it);
      }

      throw_not_found(name);
    }
    
    bool has_key(const key_type &name) {
      unsigned int end_idx = table.size() - 1;
      for (unsigned int idx = 0; idx < table.size(); idx++) {
	scope_type &scope = table[end_idx - idx];

	typename scope_type::iterator it = scope.find(name);
	if (it != scope.end()) return true;
      }

      return false;
    }

    bool has_local(const key_type &key) {
      scope_type &scope = table.back();
      return (scope.find(key) != scope.end());
    }

    void set(const key_type &key, const entry_type &entry) { table.back().set(key, entry); }
    
    void scope_push(const std::string &scope_name = "") { table.push_back(scope_type(push_name(scope_name))); }
    
    codegen_void scope_pop(llvm::Module *module, llvm::IRBuilder<> &builder, bool destroy = true) {
      codegen_void rt = nullptr;
      if (destroy) rt = scope_destroy(module, builder);
      table.pop_back();
      pop_name();

      return rt;
    }
    
    //Generates the destructor code for this scope, without actually popping it off the stack.
    //Can be used to give a scope multiple exit points.
    codegen_void scope_destroy(llvm::Module *module, llvm::IRBuilder<> &builder) {
      return table.back().destroy(module, builder);
    }

    //Generates destructor code for the lowest N scopes.
    codegen_void scope_destroy(unsigned int N, llvm::Module *module, llvm::IRBuilder<> &builder) {
      unsigned int end = std::min(N, static_cast<unsigned int>(table.size()));
      for (unsigned int i = 0; i < end; i++) {
	Scope &scope = table[table.size() - 1 - i];
	scope.destroy(module, builder);
      }

      return nullptr;
    }

    std::string scope_name() const { return get_full_scope_name(); }

    Scope &scope() { return table.back(); }
    unsigned int depth() { return table.size(); }

    //Methods for manually iterating through the nested scopes.
    scope_iterator scope_begin() { return table.rbegin(); }
    scope_iterator scope_end() { return table.rend(); }

  private:
    std::vector<Scope> table;
    std::vector<std::string> name_stack;
    
    void throw_not_found(const key_type &name) {
      std::stringstream err_str;
      err_str << "Undeclared " << symbol_name<entry_type>() << ": " << Scope::key_to_string(name);
      throw std::runtime_error(err_str.str());
    }

    //Concatenates all the names of all the scopes on the stack.
    std::string get_full_scope_name() const {
      std::stringstream full_name_ss;
      auto it = name_stack.begin();
      full_name_ss << *it;
      ++it;

      while (it != name_stack.end()) {
	full_name_ss << "." << *it;
	++it;
      }
      
      return full_name_ss.str();
    }
    
    //Adds a new name to the stack and returns the resulting full name.
    std::string push_name(const std::string &name) {
      name_stack.push_back(name);
      return get_full_scope_name();
    }

    void pop_name() { name_stack.pop_back(); }
  };
  
  /* A table entry defining a particular variable. */
  struct variable_entry {
    llvm::Value *value;
    type_spec type;
    
    bool destroy_on_scope_exit;

    variable_entry(llvm::Value *val, type_spec t, bool do_destroy = true) :
      value(val), type(t), destroy_on_scope_exit(do_destroy) { }
    variable_entry() : value(NULL), type(nullptr), destroy_on_scope_exit(false) { }
  };

  /* A table of variable entries. */
  class variable_scope {
  public:
    
    variable_scope(const std::string &) { }


    typedef variable_entry entry_type;
    typedef std::string key_type;
    typedef boost::unordered_map<std::string, variable_entry>::iterator iterator;
    
    iterator find(const key_type &name);
    iterator begin();
    iterator end();

    entry_type &extract_entry(iterator it);
    
    void set(const key_type &name, const entry_type &entry);
    codegen_void destroy(llvm::Module *module, llvm::IRBuilder<> &builder);
    
    static std::string key_to_string(const key_type &key) { return key; }

  private:

    boost::unordered_map<std::string, variable_entry> table;

  };

  typedef scoped_symbol_table<variable_scope> variable_symbol_table;
  
  
  /* Function Symbol Table */

  struct function_argument {
    std::string name;
    type_spec type;
    bool output;

    //compares two arguments by type (ignores names)
    bool operator==(const function_argument &rhs) const;
    bool operator!=(const function_argument &rhs) const { *this != rhs; }
  };

  //Generates a function name based on the argument signature.
  std::string function_generate_name(const std::string &base_name, const std::string &scope_name, const std::vector<function_argument> &args);
  
  struct function_key {
    std::string name;
    std::vector<type_spec> arguments;
  };
  
  /* A table entry for a function declaration. */
  struct function_entry {
    llvm::Function *func;
    std::string name, full_name;
    bool external, member_function;
    
    type_spec return_type;
    std::vector<function_argument> arguments;
    
    //Returns true if this entry matches the specified return type and argument types.
    bool compare(const type_spec &rt, const std::vector<type_spec> &args);

    //Returns true if the two functions are equivalent
    bool operator==(const function_entry &rhs) const;
    
    //Creates a function entry from a name and types.
    static function_entry make_entry(const std::string &name, const std::string &scope_name,
				     const type_spec &return_type, const std::vector<function_argument> &arguments);
    function_key to_key() const;
  };
  
  /* A list of all the overloaded versions of a function. */
  class function_overload_set {
  public:

    typedef boost::unordered_map<std::string, function_entry>::iterator iterator;

    function_overload_set();

    //Adds a new variant to this set. Throws an error if the function cannot be added.
    void insert(const function_entry &entry);
    
    iterator find(const function_key &key);
    iterator begin();
    iterator end();

  private:

    bool is_viable(const function_key &key, const function_entry &entry);
    int candidate_score(const function_key &key, const function_entry &entry);

    boost::unordered_map<std::string, function_entry> versions;
  };
  
  /* A mapping from function names to all overloaded versions. */
  class function_scope {
  public:

    function_scope(const std::string &name);

    typedef boost::unordered_map<std::string, function_overload_set> table_type;
    
    class iterator {
      
      table_type::iterator name_it, end;
      function_overload_set::iterator version_it;

      friend class function_scope;
      
    public:
      
      iterator &operator++();
      function_entry &operator*();
      bool operator==(const iterator &rhs) const;
      bool operator!=(const iterator &rhs) const { return !(*this == rhs); }
      function_overload_set::iterator operator->() const { return version_it; }
    };
    
    typedef function_entry entry_type;
    typedef function_key key_type;

    iterator find(const key_type &f);
    iterator begin();
    iterator end();

    entry_type &extract_entry(iterator it);
    
    void set(const key_type &f, const entry_type &entry);
    codegen_void destroy(llvm::Module *module, llvm::IRBuilder<> &builder) { return nullptr; }

    static std::string key_to_string(const key_type &key) { return key.name; }

  private:

    table_type table;
    std::string name;

  };

  typedef scoped_symbol_table<function_scope> function_symbol_table;
  
  template<typename T>
  codegen_void destroy_entry(T &entry, llvm::Module *module, llvm::IRBuilder<> &builder);
  
};

#endif
