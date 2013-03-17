#include "compiler/ast/function.hpp"
#include <iostream>
#include <sstream>

using namespace std;
using namespace raytrace;
using namespace llvm;

/* Function Call */

raytrace::ast::func_call::func_call(const string &fname,
				    const vector<expression_ptr> &args,
				    func_symbol_table *func_symtab) :
  expression(type_code::OTHER),
  fname(fname), args(args), func_symtab(func_symtab)
{
  
}

Value *raytrace::ast::func_call::codegen(Module *module, IRBuilder<> &builder) {
  func_symbol_table::table_entry &entry = func_symtab->get(fname);
  Function *f = entry.func;
  
  if (args.size() != entry.arguments.size()) {
    stringstream err_str;
    err_str << "Expected " << entry.arguments.size() << " arguments, found " << args.size() << ".";
    throw runtime_error(err_str.str());
  }

  //check argument types
  for (unsigned int i = 0; i < args.size(); i++) {
    type_spec ats(args[i]->typecheck());
    
    if (ats != entry.arguments[i].type) {
      type_traits at = get_type_traits(ats.t);
      type_traits ft = get_type_traits(entry.arguments[i].type.t);
      stringstream err_str;
      err_str << "Error in " << i << "-th argument: Expected type '" << ft.name << "' but found type '" << at.name << "'.";
      throw runtime_error(err_str.str());
    }
  }

  //evaluate all arguments and call the function
  vector<Value*> arg_vals;
  unsigned int arg_idx =0 ;
  for (vector<expression_ptr>::iterator arg_it = args.begin(); arg_it != args.end(); arg_it++, arg_idx++) {
    if (entry.arguments[arg_idx].output) arg_vals.push_back((*arg_it)->codegen_ptr(module, builder));
    else arg_vals.push_back((*arg_it)->codegen(module, builder));
  }

  if (f->getReturnType()->isVoidTy()) return builder.CreateCall(f, arg_vals); //void returns don't get a name

  string tmp_name = fname + string("_call");
  return builder.CreateCall(f, arg_vals, tmp_name.c_str());
}

type_spec raytrace::ast::func_call::typecheck() {
  func_symbol_table::table_entry &entry = func_symtab->get(fname);
  return {entry.return_type.t};
}

/* Function Prototype */

raytrace::ast::prototype::prototype(const string &name, const type_spec &return_type,
				    const vector<function_argument> &args,
				    func_symbol_table *func_symtab) :
  global_declaration(),
  name(name), extern_name(name), return_type(return_type),
  args(args), external(false), func_symtab(func_symtab)
{

}

raytrace::ast::prototype::prototype(const string &name, const string &extern_name,
				    const type_spec &return_type, const vector<function_argument> &args,
				    func_symbol_table *func_symtab) :
  global_declaration(),
  name(name), extern_name(extern_name),
  return_type(return_type), args(args),
  external(true), func_symtab(func_symtab)
{
  
}

void raytrace::ast::prototype::set_external(const string &extern_name) {
  this->extern_name = extern_name;
  external = true;
}

Function *raytrace::ast::prototype::codegen(Module *module, IRBuilder<> &builder) {
  Function *f = check_for_entry();
  if (f) return f;

  //no previous definition, define now
  vector<Type*> arg_types;
  for (vector<function_argument>::iterator it = args.begin(); it != args.end(); it++) {
    Type *arg_type = it->type.llvm_type();
    if (it->output) arg_types.push_back(PointerType::getUnqual(arg_type));
    else arg_types.push_back(arg_type);
  }

  FunctionType *ft = FunctionType::get(return_type.llvm_type(), arg_types, false);
  f = Function::Create(ft, Function::ExternalLinkage, extern_name, module);

  //set names for all the arguments (so they can be loaded into the symbol table later)
  unsigned int arg_idx = 0;
  for (auto arg_it = f->arg_begin(); arg_it != f->arg_end(); arg_it++, arg_idx++) {
    arg_it->setName(args[arg_idx].name.c_str());
  }

  func_symbol_table::table_entry entry;
  entry.func = f;
  entry.external = external;
  entry.return_type = return_type;
  entry.arguments = args;
  
  func_symtab->set(name, entry); 
  return f;
}

Function *raytrace::ast::prototype::check_for_entry() {
  if (!func_symtab->has_name(name)) return NULL; //function has not been defined
  func_symbol_table::table_entry &entry = func_symtab->get(name);

  if (entry.external && !external) {
    stringstream err_str;
    err_str << "Function " << name << " was previously declared as external";
    throw runtime_error(err_str.str());
  }

  if (!entry.external && external) {
    stringstream err_str;
    err_str << "Function " << name << " was previously declared as local";
    throw runtime_error(err_str.str());
  }

  vector<type_spec> arg_types;
  for (auto it = args.begin(); it != args.end(); it++) {
    arg_types.push_back(it->type);
  }

  if (!entry.compare(return_type, arg_types)) {
    stringstream err_str;
    err_str << "Invalid redeclaration of function: " << name;
    throw runtime_error(err_str.str());
  }

  return entry.func;
}

/* Function */

raytrace::ast::function::function(const prototype_ptr &defn, const statement_list &body,
				  var_symbol_table *symtab, control_state *control) :
  global_declaration(),
  defn(defn), body(body), symtab(symtab), control(control)
{
  
}

AllocaInst *raytrace::ast::function::create_argument_alloca(Function *f, const function_argument &arg) {
  IRBuilder<> tmp(&f->getEntryBlock(), f->getEntryBlock().begin());
  return tmp.CreateAlloca(arg.type.llvm_type(), NULL, arg.name.c_str());
}

Function *raytrace::ast::function::codegen(Module *module, IRBuilder<> &builder) {
  Function *f = defn->codegen(module, builder);
  if (!f->empty()) {
    string err = string("Redefinition of function: ") + defn->function_name();
    throw runtime_error(err);
  }

  BasicBlock *bb = BasicBlock::Create(getGlobalContext(), "func_entry", f);
  builder.SetInsertPoint(bb);

  //define the body, first name and load up all arguments
  symtab->scope_push();
  control->push_function_rt(defn->get_return_type());

  auto arg_it = f->arg_begin();
  for (unsigned int i = 0; i < defn->num_args(); i++, arg_it++) {
    const function_argument &arg = defn->get_arg(i);
    var_symbol_table::table_entry arg_var;

    if (arg.output) {
      //output parameter, so we don't need to allocate anything
      arg_var = {arg_it, arg.type};
    }
    else {
      //create a local copy of this parameter
      AllocaInst *alloca = create_argument_alloca(f, arg);
      builder.CreateStore(arg_it, alloca);
      arg_var = {alloca, arg.type};
    }
    
    symtab->set(arg.name, arg_var);
  }
  
  body.codegen(module, builder);

  //check for a terminator
  BasicBlock *func_end = builder.GetInsertBlock();
  if (!func_end->getTerminator()) {
    //no terminator - add if return type is void, error otherwise
    if (f->getReturnType()->isVoidTy()) builder.CreateRetVoid();
    else throw runtime_error("No return statement in a non-void function");
  }
  
  control->pop_function_rt();
  symtab->scope_pop(module, builder);
  
  if (verifyFunction(*f)) {
    throw runtime_error("Error verifying function");
  }
  
  return f;
}

/* Return Statement */

raytrace::ast::return_statement::return_statement(const expression_ptr &expr, control_state *control) :
  statement(),
  expr(expr), control(control)
{

}

Value *raytrace::ast::return_statement::codegen(Module *module, IRBuilder<> &builder) {
  type_spec &expected_rt = control->return_type();
  type_spec result_t = expr->typecheck();
  if (expected_rt != result_t) {
    type_traits rtt = get_type_traits(expected_rt.t);
    type_traits expr_tt = get_type_traits(result_t.t);

    stringstream err_str;
    err_str << "Invalid return type '" << expr_tt.name << "', expected '" << rtt.name << "'";
    throw runtime_error(err_str.str());
  }

  if (expr == nullptr) return builder.CreateRetVoid();
  
  Value *v = expr->codegen(module, builder);
  return builder.CreateRet(v);
}
