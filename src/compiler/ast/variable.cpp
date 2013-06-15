#include "compiler/ast/variable.hpp"
#include "compiler/llvm_helper.hpp"

#include <stdexcept>
#include <sstream>

using namespace std;
using namespace raytrace;
using namespace llvm;

/** Variable Declaration **/

raytrace::ast::variable_decl::variable_decl(parser_state *st, const string &name,
					    const type_spec &type, const expression_ptr &init,
					    unsigned int line_no, unsigned int column_no) :
  statement(st, line_no, column_no),
  name(name), type(type), initializer(init)
{
  
}

typed_value_container ast::variable_decl::initialize_from_type(Module *module, IRBuilder<> &builder) {
  return type->initialize(module, builder);
}

codegen_void ast::variable_decl::codegen(Module *module, IRBuilder<> &builder) {
  if (variables().has_local(name)) {
    stringstream err;
    err << "Redeclaration of variable '" << name << "'.";
    return errors::make_error<errors::error_message>(err.str(), line_no, column_no);
  }
  
  typed_value_container init_value = typed_value(nullptr, type);
  bool make_copy = false;

  if (initializer) {
    init_value = initializer->codegen(module, builder);
    make_copy = initializer->bound();
  }
  else init_value = initialize_from_type(module, builder);
  
  code_value converted_init = typecast(init_value, type, make_copy, true, module, builder);
  boost::function<codegen_void (value &)> op = [this, module, &builder] (value &arg) -> codegen_void {
    Value *val = arg.extract_value();
    Value *ptr = type->allocate(module, builder);
    if (val) {
      type->store(val, ptr, module, builder);
    }
    
    variable_symbol_table::entry_type entry(ptr, type);
    variables().set(name, entry);

    return empty_type();
  };
  
  return errors::codegen_call<code_value, codegen_void>(converted_init, op);
}

/** Global Variable Declaration **/

raytrace::ast::global_variable_decl::global_variable_decl(parser_state *st,
							  const string &name, const type_spec &type) :
  global_declaration(st),
  name(name), type(type)
{

}

raytrace::codegen_value raytrace::ast::global_variable_decl::codegen(llvm::Module *module, llvm::IRBuilder <> &builder) {
  variable_scope &global_scope = global_variables();
  variable_scope::iterator it = global_scope.find(name);
  
  if (it != global_scope.end()) {
    stringstream err;
    err << "Redeclaration of variable '" << name << "'.";
    return errors::make_error<errors::error_message>(err.str(), line_no, column_no);
  }
  
  GlobalVariable *gv = new GlobalVariable(type->llvm_type(), false, GlobalValue::ExternalLinkage, NULL, full_name());
  module->getGlobalList().push_back(gv);

  variable_symbol_table::entry_type entry(gv, type);
  global_scope.set(name, entry);

  exports::variable_export exp;
  exp.name = name;
  exp.full_name = full_name();
  exp.type = type;
  state->exports.add_variable(exp);
  
  return gv;
}

string ast::global_variable_decl::full_name() {
  stringstream ss;
  ss << state->modules.scope_name() << "." << name;
  return ss.str();
}


/** Variable Reference **/

raytrace::ast::variable_ref::variable_ref(parser_state *st, const string &name) :
  expression(st), name(name)
{
  
}

typecheck_value ast::variable_ref::typecheck() {
  return variable_type_lookup(name);
}

typed_value_container ast::variable_ref::codegen(Module *module, IRBuilder<> &builder) {
  boost::function<typed_value_container (typed_value &)> op = [module, &builder] (typed_value &val) -> typed_value_container {
    if (val.get<0>().type() == value::LLVM_VALUE) {
      type_spec t = val.get<1>();
      return typed_value(t->load(val.get<0>().extract_value(), module, builder), t);
    }
    
    //no need to generate a load, return directly
    return val;
  };
    
  typed_value_container addr = lookup_typed_var();
  return errors::codegen_call(addr, op);
}

typed_value_container ast::variable_ref::lookup_typed_var() {
  return variable_lookup(name);
}

typed_value_container ast::variable_ref::codegen_ptr(Module *module, IRBuilder<> &builder) {
  return lookup_typed_var();
}

code_value ast::variable_ref::codegen_module() {
  typed_value_container var = lookup_typed_var();
  return errors::codegen_call<typed_value_container, code_value>(var, [this] (typed_value &v) -> code_value {
      if (v.get<0>().type() == value::MODULE_VALUE) return v.get<0>();
      return errors::make_error<errors::error_message>("Name does not refer to a module.", line_no, column_no);
    });
}

/** Type Constructors **/

raytrace::ast::type_constructor::type_constructor(parser_state *st, const type_spec &type, const vector<expression_ptr> &args) :
  expression(st), type(type), args(args)
{

}

typed_value_container ast::type_constructor::codegen(Module *module, IRBuilder<> &builder) {
  typed_value_vector arg_values;
  vector< typed_value_container > to_destroy;
  
  for (unsigned int i = 0; i < args.size(); i++) {
    typed_value_container val = get_arg(i, module, builder);
    arg_values = errors::codegen_vector_push_back(arg_values, val);
    if (!args[i]->bound()) to_destroy.push_back(val);
  }

  typed_value_container result = type->create(module, builder, arg_values);

  for (auto it = to_destroy.begin(); it != to_destroy.end(); ++it) {
    expression::destroy_unbound(*it, module, builder);
  }
  
  return result;
}

typed_value_container ast::type_constructor::get_arg(int i, Module *module, IRBuilder<> &builder) {
  return args[i]->codegen(module, builder);
}

/** Field Selection **/

ast::field_selection::field_selection(parser_state *st, const string &field, const expression_ptr &expr,
				      unsigned int line_no, unsigned int column_no) :
  expression(st), field(field), expr(expr)
{

}

typecheck_value ast::field_selection::typecheck() {
  typecheck_value expr_type = expr->typecheck();

  return errors::codegen_call(expr_type, [this] (type_spec &t) -> typecheck_value {
      if (*t != *state->types["module"]) { return t->field_type(field); }

      code_value module_val = expr->codegen_module();
      return typecheck_module_member(module_val);
    });
}

typecheck_value ast::field_selection::typecheck_module_member(code_value &module_val) {
  return errors::codegen_call<code_value, typecheck_value>(module_val, [this] (value &v) -> typecheck_value {
      module_ptr module = v.extract_module();
      
      //search module variables
      auto var_it = module->variables.find(field);
      if (var_it != module->variables.end()) return var_it->second.type;

      //search sub-module names
      auto mod_it = module->modules.find(field);
      if (mod_it != module->modules.end()) return state->types["module"];

      stringstream err_ss;
      err_ss << "Unable to find name '" << field << "' in the given module.";
      return errors::make_error<errors::error_message>(err_ss.str(), line_no, column_no);
    });
}

typed_value ast::field_selection::load_global(Value *ptr, type_spec &type,
					      Module *module, IRBuilder<> &builder) {
  return typed_value(type->load(ptr, module, builder), type);
}

typed_value_container ast::field_selection::codegen(Module *module, IRBuilder<> &builder) {
  typed_value_container value = expr->codegen(module, builder);
  
  boost::function<typed_value_container (typed_value &)> access = [this, module, &builder] (typed_value &arg) -> typed_value_container {
    type_spec ts = arg.get<1>();
    if (*ts != *state->types["module"]) {
      Value *val = arg.get<0>().extract_value();;
      return ts->access_field(field, val, module, builder);
    }

    //perform module member lookup
    module_ptr mod = arg.get<0>().extract_module();

    //search module variables
    auto var_it = mod->variables.find(field);
    if (var_it != mod->variables.end()) return load_global(var_it->second.value, var_it->second.type,
							   module, builder);

    //search sub-modules
    auto mod_it = mod->modules.find(field);
    if (mod_it != mod->modules.end()) return typed_value(mod_it->second, state->types["module"]);

    stringstream err_ss;
    err_ss << "Unable to find name '" << field << "' in the given module.";
    return errors::make_error<errors::error_message>(err_ss.str(), line_no, column_no);
  };

  typed_value_container result = errors::codegen_call(value, access);
  if (!expr->bound()) ast::expression::destroy_unbound(value, module, builder);
  return result;
}

typed_value_container ast::field_selection::codegen_ptr(Module *module, IRBuilder<> &builder) {
  typed_value_container ptr = expr->codegen_ptr(module, builder);
  
  boost::function<typed_value_container (typed_value &)> access = [this, module, &builder] (typed_value &arg) -> typed_value_container {
    type_spec ts = arg.get<1>();
    if (*ts != *state->types["module"]) {
      Value *val = arg.get<0>().extract_value();
      return ts->access_field_ptr(field, val, module, builder);
    }

    //perform module member lookup
    module_ptr mod = arg.get<0>().extract_module();

    //search module variables
    auto var_it = mod->variables.find(field);
    if (var_it != mod->variables.end()) return typed_value(var_it->second.value, var_it->second.type);

    //search sub-modules
    auto mod_it = mod->modules.find(field);
    if (mod_it != mod->modules.end()) return typed_value(mod_it->second, state->types["module"]);

    stringstream err_ss;
    err_ss << "Unable to find name '" << field << "' in the given module.";
    return errors::make_error<errors::error_message>(err_ss.str(), line_no, column_no);
  };

  return errors::codegen_call(ptr, access);
}

code_value ast::field_selection::codegen_module() {
  code_value expr_mod = expr->codegen_module();
  return errors::codegen_call(expr_mod, [this] (value &mod) -> code_value {
      module_ptr m = mod.extract_module();
      
      //search module variables
      auto var_it = m->variables.find(field);
      if (var_it != m->variables.end()) return errors::make_error<errors::error_message>("Expression cannot be converted to a module", line_no, column_no);
      
      //search sub-modules
      auto mod_it = m->modules.find(field);
      if (mod_it != m->modules.end()) return mod_it->second;
      
      stringstream err_ss;
      err_ss << "Unable to find module named '" << field << "' in the given module.";
      return errors::make_error<errors::error_message>(err_ss.str(), line_no, column_no);
    });
}

/** Element Selection **/

ast::element_selection::element_selection(parser_state *st, const expression_ptr &expr, const expression_ptr &idx_expr,
					  unsigned int line_no, unsigned int column_no) :
  expression(st, line_no, column_no),
  expr(expr), idx_expr(idx_expr)
{
  
}

typecheck_value ast::element_selection::typecheck() {
  typecheck_value expr_type = expr->typecheck();

  return errors::codegen_call(expr_type, [this] (type_spec &t) -> typecheck_value {
      return t->element_type();
    });
}

typed_value_container ast::element_selection::codegen(Module *module, IRBuilder<> &builder) {
  if (expr->bound()) {
    //we know the resulting value is located in memory, so no need to load it
    //this allows us to do array access without loading the entire array
    typed_value_container elem_ptr = codegen_ptr(module, builder);

    boost::function<typed_value_container (typed_value &)> loader = [module, &builder] (typed_value &ptr) -> typed_value_container {
      Value *elem = ptr.get<1>()->load(ptr.get<0>().extract_value(), module, builder);
      return typed_value(elem, ptr.get<1>());
    };

    return errors::codegen_call(elem_ptr, loader);
  }
  else {
    typed_value_container expr_val = expr->codegen(module, builder);
    typed_value_container idx = idx_expr->codegen(module, builder);
    
    typedef errors::argument_value_join<typed_value_container, typed_value_container>::result_value_type typed_value_pair;
    boost::function<typed_value_container (typed_value_pair &)> access = [this, &expr_val, &idx, module, &builder] (typed_value_pair &args) -> typed_value_container {
      type_spec expr_type = args.get<0>().get<1>();
      type_spec idx_type = args.get<1>().get<1>();
      
      if (*idx_type != *state->types["int"]) {
	return errors::make_error<errors::error_message>("Array index must be an integer.", line_no, column_no);
      }
      
      Value *arr_val = args.get<0>().get<0>().extract_value();
      Value *idx_val = args.get<1>().get<0>().extract_value();
      
      typed_value_container result = expr_type->access_element(arr_val, idx_val, module, builder);
      
      if (!idx_expr->bound()) destroy_unbound(idx, module, builder);
      if (!expr->bound()) destroy_unbound(expr_val, module, builder);
      return result;
    };

    return errors::codegen_call_args(access, expr_val, idx);
  }
}

typed_value_container ast::element_selection::codegen_ptr(Module *module, IRBuilder<> &builder) {
  typed_value_container ptr = expr->codegen_ptr(module, builder);
  typed_value_container idx = idx_expr->codegen(module, builder);

  typedef errors::argument_value_join<typed_value_container, typed_value_container>::result_value_type typed_value_pair;
  boost::function<typed_value_container (typed_value_pair &)> access = [this, &idx, module, &builder] (typed_value_pair &args) -> typed_value_container {
    type_spec ptr_type = args.get<0>().get<1>();
    type_spec idx_type = args.get<1>().get<1>();
    
    if (*idx_type != *state->types["int"]) {
      return errors::make_error<errors::error_message>("Array index must be an integer.", line_no, column_no);
    }

    Value *ptr_val = args.get<0>().get<0>().extract_value();
    Value *idx_val = args.get<1>().get<0>().extract_value();

    typed_value_container result = ptr_type->access_element_ptr(ptr_val, idx_val, module, builder);
    
    if (!idx_expr->bound()) destroy_unbound(idx, module, builder);
    return result;
  };

  return errors::codegen_call_args(access, ptr, idx);
}
