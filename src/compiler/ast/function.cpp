#include "compiler/ast/function.hpp"
#include <iostream>
#include <sstream>

using namespace std;
using namespace raytrace;
using namespace llvm;

/* Function Call */

raytrace::ast::func_call::func_call(parser_state *st, const string &fname,
				    const vector<expression_ptr> &args) :
  expression(st, st->types["void"]),
  fname(fname), args(args)
{
  
}

function_key ast::func_call::get_key() const {
  function_key fkey;
  fkey.name = fname;
  for (auto it = args.begin(); it != args.end(); it++) fkey.arguments.push_back((*it)->typecheck());
  return fkey;
}

codegen_value raytrace::ast::func_call::codegen(Module *module, IRBuilder<> &builder) {  
  function_symbol_table::entry_type *entry = nullptr;
  try { entry = &state->functions.get(get_key()); }
  catch (compile_error &e) { return e; }
  
  Function *f = entry->func;
  bool is_member_function = entry->member_function;
  
  size_t expected_size = entry->arguments.size();
  if (args.size() != expected_size) {
    stringstream err_str;
    err_str << "Expected " << expected_size << " arguments, found " << args.size() << ".";
    return compile_error(err_str.str());
  }
    
  //check argument types and evaluate
  typecheck_vector arg_types;
  codegen_vector arg_eval;

  //add the context to the argument list
  if (is_member_function) {
    codegen_value ctx_val = state->control.get_context();
    arg_eval = errors::codegen_vector_push_back(arg_eval, ctx_val);
  }

  unsigned int arg_idx = 0;
  vector< pair<typecheck_value, codegen_value> > to_destroy;

  for (vector<expression_ptr>::iterator arg_it = args.begin(); arg_it != args.end(); arg_it++, arg_idx++) {
    typecheck_value ats = (*arg_it)->typecheck_safe();
    ats = errors::codegen_call(ats, [entry, arg_idx] (type_spec &arg_ts) -> typecheck_value {
	if (*arg_ts != *entry->arguments[arg_idx].type) {
	  stringstream err_str;
	  err_str << "Error in " << arg_idx << "-th argument: Expected type '" << entry->arguments[arg_idx].type->name << "' but found type '" << arg_ts->name << "'.";
	  return compile_error(err_str.str());
	}
	
	return arg_ts;
      });
    arg_types = errors::codegen_vector_push_back(arg_types, ats);
    
    codegen_value expr = (entry->arguments[arg_idx].output ?
			  (*arg_it)->codegen_ptr(module, builder) : 
			  (*arg_it)->codegen(module, builder));
    
    if (!(*arg_it)->bound()) to_destroy.push_back(make_pair(ats, expr));
    arg_eval = errors::codegen_vector_push_back(arg_eval, expr);
  }
  
  typedef errors::argument_value_join<codegen_vector, typecheck_vector>::result_value_type arg_val_type;
  boost::function<codegen_value (arg_val_type &)> call_op = [this, &builder, f] (arg_val_type &args) -> codegen_value {
    vector<Value*> &arg_vals = args.get<0>();
    if (f->getReturnType()->isVoidTy()) return builder.CreateCall(f, arg_vals);
    
    string tmp_name = fname + string("_call");
    return builder.CreateCall(f, arg_vals, tmp_name.c_str());
  };
  
  codegen_value result = errors::codegen_call_args(call_op, arg_eval, arg_types);

  //delete any expressions used to call this function
  for (auto it = to_destroy.begin(); it != to_destroy.end(); ++it) {
    ast::expression::destroy_unbound(it->first, it->second, module, builder);
  }

  return result;
}

type_spec raytrace::ast::func_call::typecheck() {
  function_symbol_table::entry_type &entry = state->functions.get(get_key());
  return entry.return_type;
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
    
    function_symbol_table::entry_type entry = function_entry::make_entry(name, state->functions.scope_name(), return_type, args);
    string name_to_use = (external ? extern_name : entry.full_name);
    
    FunctionType *ft = FunctionType::get(return_type->llvm_type(), arg_types, false);
    Function *f = Function::Create(ft, Function::ExternalLinkage, name_to_use, module);
    
    entry.func = f;
    entry.external = external;
    entry.member_function = member_function;
    
    state->functions.set(entry.to_key(), entry);
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
  if (!state->functions.has_local(fkey)) return nullptr; //function has not been defined
  function_symbol_table::entry_type &entry = state->functions.get(fkey);

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
    
    state->variables.set(arg.name, arg_var);
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

codegen_value raytrace::ast::return_statement::codegen(Module *module, IRBuilder<> &builder) {
  if (builder.GetInsertBlock()->getTerminator()) return nullptr;
  
  state->control.set_scope_reaches_end(false);
  exit_function(module, builder);

  if (expr == nullptr) return builder.CreateRetVoid();
  
  type_spec &expected_rt = state->control.return_type();  
  typecheck_value result_t = expr->typecheck_safe();

  //make sure the expression type matches this function's return type
  boost::function<typecheck_value (type_spec &)> check = [&expected_rt] (type_spec &result) -> typecheck_value {
    if (expected_rt != result) {
      stringstream err_str;
      err_str << "Invalid return type '" << result->name << "', expected '" << expected_rt->name << "'";
      return compile_error(err_str.str());
    }
    
    return result;
  };
  
  //some values have destructors, so make a copy to ensure that we don't have invalid data (for most types this won't make a difference).
  typedef raytrace::errors::argument_value_join<codegen_value, typecheck_value>::result_value_type arg_val_type;  
  boost::function<codegen_value (arg_val_type &)> op = [module, &builder] (arg_val_type &val) -> codegen_value {
    type_spec t = val.get<1>();
    Value *copy = t->copy(val.get<0>(), module, builder);
    return builder.CreateRet(copy);
  };

  typecheck_value expr_check = errors::codegen_call(result_t, check);  
  codegen_value rt_val = expr->codegen(module, builder);
  return errors::codegen_call_args(op, rt_val, expr_check);
}
