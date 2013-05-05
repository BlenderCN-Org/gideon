#include "compiler/ast/alias.hpp"

using namespace raytrace;
using namespace std;
using namespace llvm;

ast::global_variable_alias::global_variable_alias(parser_state *st,
						  const string &full_name, const type_spec &type,
						  const string &alias_name,
						  unsigned int line_no, unsigned int column_no) :
  global_declaration(st),
  alias_name(alias_name), full_name(full_name),
  type(type)
{

}

codegen_value ast::global_variable_alias::codegen(Module *module, IRBuilder<> &) {
  variable_scope &global_scope = global_variables();
  variable_scope::iterator it = global_scope.find(alias_name);
  
  if (it != global_scope.end()) {
    stringstream err;
    err << "Redeclaration of variable '" << alias_name << "'.";
    return compile_error(err.str());
  }

  stringstream alias_ss;
  alias_ss << state->modules.scope_name() << "." << alias_name;
  
  Constant *global_var = module->getOrInsertGlobal(full_name, type->llvm_type());
  GlobalAlias *alias = new GlobalAlias(type->llvm_type()->getPointerTo(), GlobalValue::ExternalLinkage, alias_ss.str(), global_var, module);
  
  variable_symbol_table::entry_type entry(alias, type);
  global_scope.set(alias_name, entry);

  exports::variable_export exp;
  exp.name = alias_name;
  exp.full_name = full_name;
  exp.type = type;
  state->exports.add_variable(exp);
  
  return alias;
}

/* Function Alias */

ast::function_alias::function_alias(parser_state *st,
				    const function_entry &entry,
				    const string &alias_name,
				    unsigned int line_no, unsigned int column_no) :
  global_declaration(st),
  func(entry), alias_name(alias_name)
{
  
}

codegen_value ast::function_alias::codegen(Module *module, IRBuilder<> &builder) {
  function_key fkey = func.to_key();
  auto func_it = function_table().find(fkey);
  if (func_it != function_table().end()) {
    stringstream err_ss;
    err_ss << "Invalid redeclaration of function '" << alias_name << "'";
    return compile_error(err_ss.str());
  }

  Type *rt = func.return_type->llvm_type();
  vector<Type*> arg_types;
  for (auto arg_it = func.arguments.begin(); arg_it != func.arguments.end(); ++arg_it) {
    arg_types.push_back(arg_it->output ?
			arg_it->type->llvm_type()->getPointerTo() :
			arg_it->type->llvm_type());
  }

  function_entry alias_entry = function_entry::make_entry(alias_name, function_scope_name(), func.return_type, func.arguments);
  alias_entry.full_name = func.full_name;
  alias_entry.member_function = func.member_function;
  alias_entry.external = false;
  
  FunctionType *ft = FunctionType::get(rt, arg_types, false);
  Constant *func_obj = module->getOrInsertFunction(func.full_name, ft);
  alias_entry.func = cast<Function>(func_obj);
  
  function_table().set(alias_entry.to_key(), alias_entry);

  if (!func.member_function) {
    exports::function_export exp;
    exp.name = alias_entry.name;
    exp.full_name = alias_entry.full_name;
    exp.return_type = alias_entry.return_type;
    exp.arguments = alias_entry.arguments;
    state->exports.add_function(exp);
  }

  return func_obj;
}
