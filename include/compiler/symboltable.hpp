#ifndef RT_SYMBOL_TABLE_HPP
#define RT_SYMBOL_TABLE_HPP

#include "compiler/types.hpp"

#include "llvm/DerivedTypes.h"
#include "llvm/Function.h"

#include <iostream>
#include <string>
#include <map>
#include <vector>

#include <stdexcept>

namespace raytrace {

  /* A table entry defining a particular variable. */
  struct variable_entry {
    llvm::Value *value;
    type_spec type;
  };

  struct function_argument {
    std::string name;
    type_spec type;
    bool output;
  };
  
  /* A table entry for a function declaration. */
  struct function_entry {
    llvm::Function *func;
    bool external;
    
    type_spec return_type;
    std::vector<function_argument> arguments;
    
    //Returns true if this entry matches the specified return type and argument types.
    bool compare(const type_spec &rt, const std::vector<type_spec> &args);
  };

  template<typename T>
  void destroy_entry(T &entry, llvm::Module *module, llvm::IRBuilder<> &builder);

  //Returns a string name that the symbol table uses for error generation.
  template<typename SymType>
  std::string symbol_name();
  
  /* A generic symbol table with support for scope. */
  template<typename SymType>
  class symbol_table {
  public:

    symbol_table() : table(1) {}
    
    typedef SymType table_entry;
    typedef std::map<std::string, table_entry> symtab_scope;
    
    //Returns a reference to an entry in the table, throwing an exception if not found.
    table_entry &get(const std::string &name) {
      for (unsigned int idx = 0; idx < table.size(); idx++) {
	symtab_scope &scope = table[table.size() - 1 - idx];
	auto it = scope.find(name);
	if (it != scope.end()) return it->second;
      }
      
      std::string err_str = std::string("Undeclared ") + symbol_name<SymType>() + std::string(": ") + name;
      throw std::runtime_error(err_str);
    }

    bool has_local_name(const std::string &name) {
      auto it = table.back().find(name);
      return (it != table.back().end());
    }

    bool has_name(const std::string &name) {
      for (unsigned int idx = 0; idx < table.size(); idx++) {
	symtab_scope &scope = table[table.size() - 1 - idx];
	if (scope.find(name) != scope.end()) return true;
      }

      return false;
    }
    
    void set(const std::string &name, const table_entry &entry) { table.back()[name] = entry; }

    void scope_push() { table.push_back(symtab_scope()); }

    void scope_pop(llvm::Module *module, llvm::IRBuilder<> &builder) {
      destroy_entries(module, builder);
      table.pop_back();
    }

  private:
    
    std::vector<symtab_scope> table;

    void destroy_entries(llvm::Module *module, llvm::IRBuilder<> &builder) {
      //set the insertion point to before the scope's terminator
      llvm::BasicBlock *insert_bb = builder.GetInsertBlock();
      llvm::TerminatorInst *term = insert_bb->getTerminator();
      if (term) builder.SetInsertPoint(term);
      
      //generate destruction code for each entry in the scope (if applicable)
      symtab_scope &back = table.back();
      for (auto it = back.begin(); it != back.end(); it++) {
	destroy_entry(it->second, module, builder);
      }
    }
    
  };

  typedef symbol_table<variable_entry> var_symbol_table;
  typedef symbol_table<function_entry> func_symbol_table;
  
};

#endif
