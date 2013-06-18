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
    type_scope types;
    
    boost::unordered_map<std::string, module_ptr> modules;

    module_object(const std::string &name = "") : name(name), variables(name), functions(name), types(name) { }
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
    codegen_void destroy(llvm::Module *module, llvm::IRBuilder<> &builder) { return empty_type(); }

    module_ptr &get_module() { return module; }

  private:
    
    module_ptr module;

  };

  typedef scoped_symbol_table<module_scope> module_symbol_table;

  /* Types describing data exported by syntax tree nodes. */
  namespace exports {

    struct variable_export {
      std::string name, full_name;
      type_spec type;

      bool operator==(const variable_export &lhs) const;
    };

    size_t hash_value(const variable_export &v);

    struct function_export {
      std::string name, full_name;
      type_spec return_type;
      std::vector<function_argument> arguments;

      std::string get_hash_string() const;
      bool operator==(const function_export &lhs) const;
    };

    size_t hash_value(const function_export &f);

    struct module_export {
      typedef std::shared_ptr<module_export> pointer;

      std::string name;
      boost::unordered_set<variable_export> variables;
      boost::unordered_set<function_export> functions;
      boost::unordered_map<std::string, pointer> modules;

      void dump(unsigned int indent);
    };    

  };

  /* Table describing variables, functions and modules exported by a source file. */
  struct export_table {
  public:

    typedef exports::variable_export variable_export;
    typedef exports::function_export function_export;
    typedef exports::module_export module_export;

    //adds a new export to the current module
    codegen_void add_variable(const variable_export &variable);
    codegen_void add_function(const function_export &function);

    void push_module(const std::string &name);
    codegen_void pop_module();

    std::string scope_name();
    
    void dump();

    boost::unordered_map<std::string, module_export::pointer>::iterator top_begin() { return top_modules.begin(); }
    boost::unordered_map<std::string, module_export::pointer>::iterator top_end() { return top_modules.end(); }

  private:

    boost::unordered_map<std::string, module_export::pointer> top_modules;
    std::vector<module_export::pointer> module_stack;

  };

};

#endif
