#include "compiler/ast/function.hpp"
#include <iostream>
#include <sstream>

using namespace std;
using namespace raytrace;
using namespace llvm;

/* Function Call */

raytrace::ast::func_call::func_call(parser_state *st,
				    const expression_ptr &path_expr, const string &fname,
				    const vector<expression_ptr> &args) :
  expression(st, st->types["void"]),
  path_expr(path_expr), fname(fname), args(args)
{
  
}

ast::func_call::entry_or_error ast::func_call::lookup_function() {
  //determine the type of each argument
  typecheck_vector arg_types;
  for (vector<expression_ptr>::iterator arg_it = args.begin(); arg_it != args.end(); ++arg_it) {
    typecheck_value type = (*arg_it)->typecheck();
    arg_types = errors::codegen_vector_push_back(arg_types, type);
  }
  
  //lookup the function
  boost::function<entry_or_error (vector<type_spec> &)> lookup = [this] (vector<type_spec> &args) -> entry_or_error {
    function_key fkey;
    fkey.name = fname;
    for (auto it = args.begin(); it != args.end(); ++it) fkey.arguments.push_back(*it);
    
    if (path_expr == nullptr) {
      try { return &functions().get(fkey); }
      catch (compile_error &e) {  } //ignore and look up the module stack

      for (auto mod_it = state->modules.scope_begin(); mod_it != state->modules.scope_end(); ++mod_it) {
	function_scope &fscope = mod_it->get_module()->functions;
	auto func_it = fscope.find(fkey);
	if (func_it != fscope.end()) return &(*func_it);
      }

      stringstream err_ss;
      err_ss << "Undeclared function '" << fname << "'";
      return compile_error(err_ss.str());
    }
    else {
      code_value module_path = path_expr->codegen_module();
      return errors::codegen_call<code_value, entry_or_error>(module_path, [&fkey] (value &val) -> entry_or_error {
	  module_ptr m = val.extract_module();
	  function_scope &fscope = m->functions;
	  auto func_it = fscope.find(fkey);
	  if (func_it != fscope.end()) return &(*func_it);

	  stringstream err_ss;
	  err_ss << "Could not find function named '" << fkey.name << "' in specified module.";
	  return compile_error(err_ss.str());
	});
    }
  };
  
  return errors::codegen_call<typecheck_vector, entry_or_error>(arg_types, lookup);
  
}

typecheck_value ast::func_call::typecheck() {
  entry_or_error entry = lookup_function();
  return errors::codegen_call<entry_or_error, typecheck_value>(entry, [] (function_symbol_table::entry_type *&func) -> typecheck_value {
      return func->return_type;
    });
}

typed_value_vector ast::func_call::codegen_all_args(entry_or_error &entry,
						    Module *module, IRBuilder<> &builder,
						    /* out */ vector<typed_value_container> &to_destroy) {
  boost::function<typed_value_vector (function_symbol_table::entry_type *&)> eval =
    [this, module, &builder, &to_destroy] (function_symbol_table::entry_type *&entry) -> typed_value_vector {
    
    typed_value_vector arg_eval;
    unsigned int arg_idx = 0;
    
    for (vector<expression_ptr>::iterator arg_it = args.begin(); arg_it != args.end(); ++arg_it, ++arg_idx) {
      typed_value_container arg = (entry->arguments[arg_idx].output ? 
				   (*arg_it)->codegen_ptr(module, builder) :
				   (*arg_it)->codegen(module, builder));
      if (!(*arg_it)->bound()) to_destroy.push_back(arg);
      arg_eval = errors::codegen_vector_push_back(arg_eval, arg);
    };
  
    //add the context to the front argument list
    if (entry->member_function) {
      typed_value_container ctx_val = typed_value(state->control.get_context(), state->types["context_ptr"]);

      typedef errors::argument_value_join<typed_value_vector, typed_value_container>::result_value_type add_ctx_arg_type;
      boost::function<typed_value_vector (add_ctx_arg_type&)> add_ctx_func = [&arg_eval] (add_ctx_arg_type &arg) -> typed_value_vector {
	vector<typed_value> &result = arg.get<0>();
	result.insert(result.begin(), arg.get<1>());
	return result;
      };

      arg_eval = errors::codegen_call_args(add_ctx_func, arg_eval, ctx_val);
    }
    
    return arg_eval;
  };

  return errors::codegen_call<entry_or_error, typed_value_vector>(entry, eval);
}

typed_value_container ast::func_call::codegen(Module *module, IRBuilder<> &builder) {  
  //lookup the function
  entry_or_error func = lookup_function();

  //evaluate all arguments
  vector<typed_value_container> to_destroy;
  typed_value_vector arg_eval = codegen_all_args(func, module, builder, to_destroy);

  typedef errors::argument_value_join<entry_or_error, typed_value_vector>::result_value_type arg_val_type;
  boost::function<typed_value_container (arg_val_type &)> call_func = [this, module, &builder] (arg_val_type &call_args) -> typed_value_container {
    function_symbol_table::entry_type *entry = call_args.get<0>();
    vector<typed_value> &args = call_args.get<1>();

    Function *f = entry->func;
    bool is_member_function = entry->member_function;
  
    size_t expected_size = entry->arguments.size();
    size_t found_args = args.size();
    if (is_member_function) found_args--;

    if (found_args != expected_size) {
      stringstream err_str;
      err_str << "Expected " << expected_size << " arguments, found " << found_args << ".";
      return compile_error(err_str.str());
    }

    vector<Value*> arg_vals;
    unsigned int arg_idx = 0;

    auto arg_it = args.begin();
    if (is_member_function) {
      arg_vals.push_back(args[0].get<0>().extract_value());
      ++arg_it;
    }
    
    while (arg_it != args.end()) {
      typed_value &arg = (*arg_it);
      type_spec arg_ts = arg.get<1>();

      if (*arg_ts != *entry->arguments[arg_idx].type) {
	stringstream err_str;
	err_str << "Error in " << arg_idx << "-th argument: Expected type '" << entry->arguments[arg_idx].type->name << "' but found type '" << arg_ts->name << "'.";
	return compile_error(err_str.str());
      }

      arg_vals.push_back(arg.get<0>().extract_value());

      ++arg_it;
      ++arg_idx;
    }
    
    if (f->getReturnType()->isVoidTy()) return typed_value(builder.CreateCall(f, arg_vals), entry->return_type);
    
    string tmp_name = fname + string("_call");
    return typed_value(builder.CreateCall(f, arg_vals, tmp_name.c_str()), entry->return_type);
  };
  
  typed_value_container result = errors::codegen_call_args(call_func, func, arg_eval);

  //delete any expressions used to call this function
  for (auto it = to_destroy.begin(); it != to_destroy.end(); ++it) {
    ast::expression::destroy_unbound(*it, module, builder);
  }
  
  return result;
}

/* Function Prototype */

raytrace::ast::prototype::prototype(parser_state *st, const string &name, const type_spec &return_type,
				    const vector<function_argument> &args) :
  global_declaration(st),
  name(name), extern_name(name), return_type(return_type),
  args(args), external(false), member_function(false)
{

}

raytrace::ast::prototype::prototype(parser_state *st, const string &name, const string &extern_name,
				    const type_spec &return_type, const vector<function_argument> &args) :
  global_declaration(st),
  name(name), extern_name(extern_name),
  return_type(return_type), args(args),
  external(true), member_function(false)
{
  
}

void raytrace::ast::prototype::set_external(const string &extern_name) {
  this->extern_name = extern_name;
  external = true;
}

codegen_value raytrace::ast::prototype::codegen(Module *module, IRBuilder<> &builder) {
  codegen_value f = check_for_entry();

  auto gen_func = [this, module, &builder] (Value *&f_val) -> codegen_value {
    if (f_val) return cast<Function>(f_val);
    
    //no previous definition, define now
    member_function = state->control.has_context();
    vector<Type*> arg_types;
    
    if (member_function) arg_types.push_back(state->control.get_context_type()); //add an extra argument if this function has an associated context
    
    for (vector<function_argument>::iterator it = args.begin(); it != args.end(); it++) {
      Type *arg_type = it->type->llvm_type();
      if (it->output) arg_types.push_back(PointerType::getUnqual(arg_type));
      else arg_types.push_back(arg_type);
    }
    
    function_symbol_table::entry_type entry = function_entry::make_entry(name, function_scope_name(), return_type, args);
    string name_to_use = (external ? extern_name : entry.full_name);
    
    FunctionType *ft = FunctionType::get(return_type->llvm_type(), arg_types, false);
    Function *f = Function::Create(ft, Function::ExternalLinkage, name_to_use, module);
    
    entry.func = f;
    entry.external = external;
    entry.member_function = member_function;
    
    function_table().set(entry.to_key(), entry);
    return f;
  };

  return errors::codegen_call(f, gen_func);
}

function_key ast::prototype::get_key() const {
  function_key fkey;
  fkey.name = name;
  for (auto it = args.begin(); it != args.end(); it++) fkey.arguments.push_back(it->type);
  return fkey;
}

codegen_value raytrace::ast::prototype::check_for_entry() {
  function_key fkey = get_key();
  auto func_it = function_table().find(fkey);

  if (func_it == function_table().end()) return nullptr; //function has not been defined
  function_symbol_table::entry_type &entry = *func_it;

  if (entry.external && !external) {
    stringstream err_str;
    err_str << "Function " << name << " was previously declared as external";
    return compile_error(err_str.str());
  }

  if (!entry.external && external) {
    stringstream err_str;
    err_str << "Function " << name << " was previously declared as local";
    return compile_error(err_str.str());
  }

  vector<type_spec> arg_types;
  for (auto it = args.begin(); it != args.end(); it++) {
    arg_types.push_back(it->type);
  }

  if (!entry.compare(return_type, arg_types)) {
    stringstream err_str;
    err_str << "Invalid redeclaration of function: " << name;
    return runtime_error(err_str.str());
  }

  return entry.func;
}

/* Function */

raytrace::ast::function::function(parser_state *st, const prototype_ptr &defn, const statement_list &body) :
  global_declaration(st),
  defn(defn), body(body)
{
  
}

AllocaInst *raytrace::ast::function::create_argument_alloca(Function *f, const function_argument &arg) {
  IRBuilder<> tmp(&f->getEntryBlock(), f->getEntryBlock().begin());
  return tmp.CreateAlloca(arg.type->llvm_type(), NULL, arg.name.c_str());
}

codegen_value raytrace::ast::function::codegen(Module *module, IRBuilder<> &builder) {
  codegen_value ptype = defn->codegen(module, builder);
  auto op = [this, module, &builder] (Value *&val) -> codegen_value {
    return create_function(val, module, builder);
  };
  return errors::codegen_call(ptype, op);
}

codegen_value raytrace::ast::function::create_function(Value *& val, Module *module, IRBuilder<> &builder) {
  Function *f = cast<Function>(val);
  if (!f->empty()) {
    string err = string("Redefinition of function: ") + defn->function_name();
    return compile_error(err);
  }
  
  bool is_member_function = defn->is_member_function();
  BasicBlock *bb = BasicBlock::Create(getGlobalContext(), "func_entry", f);
  builder.SetInsertPoint(bb);
  
  //define the body, first name and load up all arguments
  push_function(defn->get_return_type());

  auto arg_it = f->arg_begin();
  if (is_member_function) {
    //load the context into the symbol table
    state->control.set_context(arg_it);
    state->control.load_context(module, builder);
    arg_it++;
  }

  for (unsigned int i = 0; i < defn->num_args(); i++, arg_it++) {
    const function_argument &arg = defn->get_arg(i);
    variable_symbol_table::entry_type arg_var;

    if (arg.output) {
      //output parameter, so we don't need to allocate anything
      arg_var = variable_symbol_table::entry_type(arg_it, arg.type, false);
    }
    else {
      //create a local copy of this parameter
      AllocaInst *alloca = create_argument_alloca(f, arg);
      builder.CreateStore(arg_it, alloca);
      arg_var = variable_symbol_table::entry_type(alloca, arg.type, false);
    }
    
    variables().set(arg.name, arg_var);
  }
  
  //codegen the body and exit the function's scope
  codegen_void body_result = body.codegen(module, builder);
  pop_function(module, builder);
  
  //assuming everything worked, add a terminator and return from the function
  typedef errors::argument_value_join<codegen_void>::result_value_type arg_val_type;
  boost::function<codegen_value (arg_val_type &)> error_ret = [this, module, &builder, f] (arg_val_type &) -> codegen_value {
    //check for a terminator
    BasicBlock *func_end = builder.GetInsertBlock();
    if (!func_end->getTerminator()) {
      f->dump();
      //no terminator - add if return type is void, error otherwise
      if (f->getReturnType()->isVoidTy()) builder.CreateRetVoid();
      else return compile_error("No return statement in a non-void function");
    }
    
    if (verifyFunction(*f)) return compile_error("Error verifying function");
    return f;
  };
  
  return errors::codegen_call_args(error_ret, body_result);
}

/* Return Statement */

raytrace::ast::return_statement::return_statement(parser_state *st, const expression_ptr &expr) :
  statement(st),
  expr(expr)
{

}

codegen_void raytrace::ast::return_statement::codegen(Module *module, IRBuilder<> &builder) {
  if (builder.GetInsertBlock()->getTerminator()) return nullptr;
  
  state->control.set_scope_reaches_end(false);
  exit_function(module, builder);

  if (expr == nullptr) {
    builder.CreateRetVoid();
    return nullptr;
  }
  
  typed_value_container rt_val = expr->codegen(module, builder);
  type_spec &expected_rt = state->control.return_type();  
  
  //make sure the expression type matches this function's return type
  boost::function<codegen_void (typed_value &)> check = [&expected_rt, module, &builder] (typed_value &result) -> codegen_void {
    type_spec t = result.get<1>();
    if (expected_rt != t) {
      stringstream err_str;
      err_str << "Invalid return type '" << result.get<1>()->name << "', expected '" << expected_rt->name << "'";
      return compile_error(err_str.str());
    }
    
    //some values have destructors, so make a copy to ensure that we don't have invalid data (for most types this won't make a difference).
    Value *copy = t->copy(result.get<0>().extract_value(), module, builder);
    builder.CreateRet(copy);
    return nullptr;
  };
  
  return errors::codegen_call<typed_value_container, codegen_void>(rt_val, check);
}
