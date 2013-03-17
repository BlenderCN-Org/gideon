#include "compiler/symboltable.hpp"

#include <sstream>
#include <stdexcept>

using namespace std;
using namespace raytrace;
using namespace llvm;

namespace raytrace {
  
  template<>
  string symbol_name<variable_entry>() { return "variable"; }

  template<>
  string symbol_name<function_entry>() { return "function"; }

  template<>
  void destroy_entry<variable_entry>(variable_entry &entry, Module *module, IRBuilder<> &builder) {
    entry.type.destroy(entry.value, module, builder);
  }

  template<>
  void destroy_entry<function_entry>(function_entry &, Module *, IRBuilder<> &) { }
};

bool raytrace::function_entry::compare(const type_spec &rt, const vector<type_spec> &args) {
  if (return_type != rt) return false;
  if (arguments.size() != args.size()) return false;
  
  for (unsigned int i = 0; i < arguments.size(); i++) {
    if (arguments[i].type != args[i]) return false;
  }
  
  return true;
}
