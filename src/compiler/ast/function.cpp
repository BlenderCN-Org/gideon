/*

  Copyright 2013 Curtis Andrus

  This file is part of Gideon.

  Gideon is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  Gideon is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with Gideon.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "compiler/ast/function.hpp"
#include "compiler/debug.hpp"

#include <iostream>
#include <sstream>

using namespace std;
using namespace raytrace;
using namespace llvm;

/* Function Call */

raytrace::ast::func_call::func_call(parser_state *st,
				    const expression_ptr &path_expr, const string &fname,
				    const vector<expression_ptr> &args,
				    unsigned int line_no, unsigned int column_no) :
  expression(st, st->types["void"], line_no, column_no),
  path_expr(path_expr), fname(fname), args(args)
{
  
}

ast::ast_node::entry_or_error ast::func_call::lookup_function() {
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
      return function_lookup(fkey);
    }
    else {
      code_value module_path = path_expr->codegen_module();
      return errors::codegen_call<code_value, entry_or_error>(module_path, [this, &fkey] (value &val) -> entry_or_error {
	  module_ptr m = val.extract_module();
	  function_scope &fscope = m->functions;
	  auto func_it = fscope.find_best(fkey, state->type_conversions);
	  if (func_it != fscope.end()) return &(*func_it);

	  stringstream err_ss;
	  err_ss << "Could not find function named '" << fkey.name << "' in specified module.";
	  return errors::make_error<errors::error_message>(err_ss.str(), line_no, column_no);
	});
    }
  };
  
  return errors::codegen_call<typecheck_vector, entry_or_error>(arg_types, lookup);
}

bool ast::func_call::check_for_array_reference_cast(const function_argument &arg) const {
  return (arg.type->is_array() && (arg.type == state->types.get_array_ref(arg.type->element_type())));
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
      bool need_pointer = (entry->arguments[arg_idx].output || check_for_array_reference_cast(entry->arguments[arg_idx]));
      typed_value_container arg = (need_pointer ? 
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
	vector<typed_value> &result = errors::get<0>(arg);
	result.insert(result.begin(), errors::get<1>(arg));
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
  
  boost::function<typed_value_container (function_symbol_table::entry_type *&,
					 vector<typed_value> &)> call_func = [this, module, &builder] (function_symbol_table::entry_type *&entry,
												       vector<typed_value> &args) -> typed_value_container {
    Function *f = entry->func;
    bool is_member_function = entry->member_function;
    
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

      if (entry->arguments[arg_idx].output) arg_vals.push_back(arg.get<0>().extract_value());
      else if (check_for_array_reference_cast(entry->arguments[arg_idx])) {
	code_value array_ref = conversion_llvm::array_to_array_ref(arg.get<0>().extract_value(), arg_ts->llvm_type(),
								   entry->arguments[arg_idx].type,
								   module, builder);
	errors::codegen_call<code_value, codegen_void>(array_ref,
						       [&] (value &v) -> codegen_void {
							 arg_vals.push_back(v.extract_value());
							 return empty_type();
						       });
      }
      else {
	code_value cast_arg_val = typecast(arg.get<0>().extract_value(),
					   arg_ts, entry->arguments[arg_idx].type,
					   false, false,
					   module, builder);
	
	errors::codegen_call<code_value, codegen_void>(cast_arg_val,
						       [&] (value &v) -> codegen_void {
							 arg_vals.push_back(v.extract_value());
							 return empty_type();
						       });
      }
      
      ++arg_it;
      ++arg_idx;
    }

    BasicBlock *lp = generate_landing_pad(module);
    BasicBlock *next = BasicBlock::Create(getGlobalContext(), "invoke.next", state->control.get_function_state().func);
    
    if (f->getReturnType()->isVoidTy()) {
      typed_value rt(builder.CreateInvoke(f, next, lp, arg_vals), entry->return_type);
      builder.SetInsertPoint(next);
      return rt;
    }
    
    string tmp_name = fname + string("_call");
    typed_value rt(builder.CreateInvoke(f, next, lp, arg_vals, tmp_name.c_str()), entry->return_type);
    builder.SetInsertPoint(next);
    return rt;
  };
  
  typed_value_container result = errors::codegen_apply(call_func, func, arg_eval);

  //delete any expressions used to call this function
  for (auto it = to_destroy.begin(); it != to_destroy.end(); ++it) {
    ast::expression::destroy_unbound(*it, module, builder);
  }
  
  return result;
}

/* Function Prototype */

raytrace::ast::prototype::prototype(parser_state *st, const string &name, const type_expr_ptr &return_type,
				    const vector<function_parameter> &args,
				    exports::function_export::export_type exp_type,
				    unsigned int line_no, unsigned int column_no) :
  global_declaration(st, line_no, column_no),
  name(name), extern_name(name), return_type(return_type),
  args(args), external(false), member_function(false),
  exp_type(exp_type)
{

}

raytrace::ast::prototype::prototype(parser_state *st, const string &name, const string &extern_name,
				    const type_expr_ptr &return_type, const vector<function_parameter> &args,
				    unsigned int line_no, unsigned int column_no) :
  global_declaration(st, line_no, column_no),
  name(name), extern_name(extern_name),
  return_type(return_type), args(args),
  external(true), member_function(false),
  exp_type(exports::function_export::export_type::FOREIGN)
{
  
}

void raytrace::ast::prototype::set_external(const string &extern_name) {
  this->extern_name = extern_name;
  external = true;
}

codegen_value raytrace::ast::prototype::codegen(Module *module, IRBuilder<> &builder) {
  function_gen_value func = codegen_entry(module, builder);
  return errors::codegen_call<function_gen_value, codegen_value>(func, [] (function_symbol_table::entry_type &entry) -> codegen_value {
      return entry.func;
    });
}

function_key ast::prototype::get_key(const vector<type_spec> &arg_types) const {
  function_key fkey;
  fkey.name = name;
  for (auto it = arg_types.begin(); it != arg_types.end(); ++it) fkey.arguments.push_back(*it);
  return fkey;
}

ast::ast_node::entry_or_error raytrace::ast::prototype::check_for_entry(const vector<type_spec> &arg_types, const type_spec &return_ty) {
  function_key fkey = get_key(arg_types);
  auto func_it = function_table().find(fkey);

  if (func_it == function_table().end()) return nullptr; //function has not been defined
  function_symbol_table::entry_type &entry = *func_it;

  if (entry.external && !external) {
    stringstream err_str;
    err_str << "Function " << name << " was previously declared as external";
    return errors::make_error<errors::error_message>(err_str.str(), line_no, column_no);
  }

  if (!entry.external && external) {
    stringstream err_str;
    err_str << "Function " << name << " was previously declared as local";
    return errors::make_error<errors::error_message>(err_str.str(), line_no, column_no);
  }
  
  if (!entry.compare(return_ty, arg_types)) {
    stringstream err_str;
    err_str << "Invalid redeclaration of function: " << name;
    return errors::make_error<errors::error_message>(err_str.str(), line_no, column_no);
  }

  return &entry;
}

ast::prototype::function_gen_value ast::prototype::codegen_entry(Module *module, IRBuilder<> &builder) {
  typecheck_vector arg_types = get_arg_types();
  typecheck_value return_ty = return_type->codegen_type();

  boost::function<function_gen_value (vector<type_spec> &, type_spec &)> gen_op = [this, module, &builder] (vector<type_spec> &arg_types, type_spec &return_ty) -> function_gen_value {
    entry_or_error f = check_for_entry(arg_types, return_ty);
    
    auto gen_func = [this, &arg_types, &return_ty, module, &builder] (function_symbol_table::entry_type *&f_entry) -> function_gen_value {
      if (f_entry) return *f_entry;
      
      //no previous definition, define now
      member_function = state->control.has_context();
      vector<Type*> arg_llvm_types;
      
      if (member_function) arg_llvm_types.push_back(state->control.get_context_type()); //add an extra argument if this function has an associated context
      
      vector<function_argument> key_args;
      unsigned int arg_idx = 0;
      for (vector<function_parameter>::iterator it = args.begin(); it != args.end(); ++it, ++arg_idx) {
	function_argument arg;
	arg.name = it->name;
	arg.type = arg_types[arg_idx];
	arg.output = it->output;
	
	Type *arg_type = arg.type->llvm_type();
	if (arg.output) arg_llvm_types.push_back(arg.type->llvm_ptr_type());
	else arg_llvm_types.push_back(arg_type);
	
	key_args.push_back(arg);
      }
      
      function_symbol_table::entry_type entry = function_entry::make_entry(name, function_scope_name(), return_ty, key_args);
      string name_to_use = (external ? extern_name : entry.full_name);
      
      FunctionType *ft = FunctionType::get(return_ty->llvm_type(), arg_llvm_types, false);
      Function *f = Function::Create(ft, Function::ExternalLinkage, name_to_use, module);
      
      entry.func = f;
      entry.external = external;
      entry.member_function = member_function;
      
      function_table().set(entry.to_key(), entry);
      
      if (!member_function) {
	exports::function_export exp;
	exp.name = entry.name;
	exp.full_name = name_to_use;
	exp.return_type = entry.return_type;
	exp.arguments = entry.arguments;
	
	if (external) exp.type = exports::function_export::export_type::FOREIGN;
	else exp.type = exp_type;
	
	state->exports.add_function(exp);
      }
      
      return entry;
    };
    
    return errors::codegen_call<entry_or_error, function_gen_value>(f, gen_func);
  };
  
  return errors::codegen_apply(gen_op, arg_types, return_ty);
}

typecheck_vector ast::prototype::get_arg_types() {
  typecheck_vector arg_types;
  for (auto arg_it = args.begin(); arg_it != args.end(); ++arg_it) {
    typecheck_value ty = arg_it->type->codegen_type();
    arg_types = errors::codegen_vector_push_back(arg_types, ty);
  }

  return arg_types;
}

/* Function */

raytrace::ast::function::function(parser_state *st, const prototype_ptr &defn, const statement_list &body,
				  unsigned int line_no, unsigned int column_no) :
  global_declaration(st, line_no, column_no),
  defn(defn), body(body)
{
  
}

codegen_value raytrace::ast::function::codegen(Module *module, IRBuilder<> &builder) {
  ast::prototype::function_gen_value ptype = defn->codegen_entry(module, builder);
  return errors::codegen_call<ast::prototype::function_gen_value, codegen_value>(ptype, [this, module, &builder] (function_symbol_table::entry_type &entry) -> codegen_value {
      return create_function(entry, module, builder);
    });
}

codegen_value raytrace::ast::function::create_function(function_entry &entry, Module *module, IRBuilder<> &builder) {
  Function *f = entry.func;
  
  if (!f->empty()) {
    string err = string("Redefinition of function: ") + entry.name;
    return errors::make_error<errors::error_message>(err, line_no, column_no);
  }
  
  bool is_member_function = entry.member_function;
  
  //define the body, first name and load up all arguments
  bool is_entry_point = (defn->export_type() == exports::function_export::export_type::ENTRY);
  state->dbg->push_function(entry, line_no, column_no);
  push_function(entry.return_type, is_entry_point, f, module, builder);

  //generate a top-level exception handling block depending on the type of function
  if (is_entry_point)
    generate_exception_catch(module);
  else
    generate_exception_propagate(module);
  
  auto arg_it = f->arg_begin();
  if (is_member_function) {
    //load the context into the symbol table
    state->control.set_context(arg_it);
    state->control.load_context(module, builder);
    ++arg_it;
  }
  
  for (unsigned int i = 0; i < entry.arguments.size(); ++i, ++arg_it) {
    const function_argument &arg = entry.arguments[i];
    variable_symbol_table::entry_type arg_var;
    
    if (arg.output) {
      //output parameter, so we don't need to allocate anything
      arg_var = variable_symbol_table::entry_type(arg_it, arg.type, false);
    }
    else {
      //create a local copy of this parameter
      Value *alloca = arg.type->allocate(module, builder);
      arg.type->store(arg_it, alloca, module, builder);
      arg_var = variable_symbol_table::entry_type(alloca, arg.type, false);
    }
    
    variables().set(arg.name, arg_var);
  }
  
  //codegen the body and exit the function's scope
  codegen_void body_result = body.codegen(module, builder);
  
  //assuming everything worked, add a terminator and return from the function
  typedef errors::argument_value_join<codegen_void>::result_value_type arg_val_type;
  boost::function<codegen_value (arg_val_type &)> error_ret = [this, module, &builder, f] (arg_val_type &) -> codegen_value {
    //check for a terminator
    BasicBlock *func_end = builder.GetInsertBlock();
    if (!func_end->getTerminator()) {
      //no terminator - add if return type is void, error otherwise
      if (f->getReturnType()->isVoidTy()) generate_return_branch(module, builder);
      else return errors::make_error<errors::error_message>("No return statement in a non-void function", line_no, column_no);
    }

    state->dbg->pop();
    pop_function(module, builder);
    
    if (verifyFunction(*f)) return errors::make_error<errors::error_message>("Error verifying function", line_no, column_no);
    return f;
  };
  
  return errors::codegen_call_args(error_ret, body_result);
}

/* Return Statement */

raytrace::ast::return_statement::return_statement(parser_state *st, const expression_ptr &expr,
						  unsigned int line_no, unsigned int column_no) :
  statement(st, line_no, column_no),
  expr(expr)
{

}

codegen_void raytrace::ast::return_statement::codegen(Module *module, IRBuilder<> &builder) {
  if (builder.GetInsertBlock()->getTerminator()) return empty_type();
  
  if (expr == nullptr) {
    generate_return_branch(module, builder);
    return empty_type();
  }
  
  typed_value_container rt_val = expr->codegen(module, builder);
  type_spec &expected_rt = state->control.get_function_state().return_ty;
  
  //make sure the expression type matches this function's return type
  boost::function<codegen_void (typed_value &)> check = [this, &expected_rt, module, &builder] (typed_value &result) -> codegen_void {
    type_spec t = result.get<1>();
    code_value rt_val = typecast(result.get<0>().extract_value(), t, expected_rt,
				 expr->bound(), true, module, builder);
    
    return errors::codegen_call<code_value, codegen_void>(rt_val, [this, module, &builder] (value &v) -> codegen_void {
	builder.CreateStore(v.extract_value(),
			    state->control.get_return_value_ptr());
	generate_return_branch(module, builder);
	return empty_type();
      });
  };
  
  return errors::codegen_call<typed_value_container, codegen_void>(rt_val, check);
}
