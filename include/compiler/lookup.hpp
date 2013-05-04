#ifndef GD_RL_LOOKUP_HPP
#define GD_RL_LOOKUP_HPP

#include "compiler/symboltable.hpp"

namespace raytrace {  
  
  /* A module is a set of functions, variables and other modules. */
  struct module_object;
  typedef std::shared_ptr<module_object> module_ptr;
  
  struct module_object {
    std::string name;
    variable_scope variables;
    function_scope functions;
    boost::unordered_map<std::string, module_ptr> modules;

    module_object(const std::string &name = "") : name(name), variables(name), functions(name) { }
  };

  /* Used to maintained a scoped list of modules. */
  class module_scope {
  public:

    module_scope(const std::string &name) : module(new module_object(name)) { }
    
    typedef module_ptr entry_type;
    typedef std::string key_type;
    typedef boost::unordered_map<key_type, entry_type>::iterator iterator;

    iterator find(const key_type &name);
    iterator begin();
    iterator end();

    entry_type &extract_entry(iterator it);
    
    void set(const key_type &name, const entry_type &entry);
    codegen_void destroy(llvm::Module *module, llvm::IRBuilder<> &builder) { }

    module_ptr &get_module() { return module; }

  private:
    
    module_ptr module;

  };

  typedef scoped_symbol_table<module_scope> module_symbol_table;
  
  //Defines a full path to a variable / function name.
  //This is used to determine what module should be searched for symbol lookups.
  typedef std::vector<std::string> name_path;

  //Performs a variable using the given path.
  typed_value_container variable_lookup(module_symbol_table &top,
					variable_symbol_table &variables,
					const name_path &path,
					llvm::Module *module, llvm::IRBuilder<> &builder);

  typecheck_value variable_type_lookup(module_symbol_table &top,
				       variable_symbol_table &variables,
				       const name_path &path);
  
};

#endif
