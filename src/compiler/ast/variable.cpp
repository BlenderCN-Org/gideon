#include "compiler/ast/variable.hpp"
#include "compiler/llvm_helper.hpp"

#include <stdexcept>
#include <sstream>

using namespace std;
using namespace raytrace;
using namespace llvm;

/** Variable Declaration **/

raytrace::ast::variable_decl::variable_decl(parser_state *st, const string &name,
					    const type_spec &type, const expression_ptr &init) :
  statement(st),
  name(name), type(type), initializer(init)
{
  
}

raytrace::codegen_value ast::variable_decl::codegen(Module *module, IRBuilder<> &builder) {
  if (state->variables.has_local(name)) {
    stringstream err;
    err << "Redeclaration of variable '" << name << "'.";
    return compile_error(err.str());
  }
  
  codegen_value init_value = nullptr;
  typecheck_value init_type = type;
  if (initializer) {
    init_value = initializer->codegen(module, builder);
    init_type = initializer->typecheck_safe();
  }
  else init_value = type->initialize(module, builder);
  
  boost::function<codegen_value (typed_value &)> op = [this, &builder] (typed_value &args) -> codegen_value {
    if (args.get<1>() != type) return compile_error("Initializer of invalid type");

    Value *val = args.get<0>();
    Value *ptr = NULL;
    ptr = CreateEntryBlockAlloca(builder, type->llvm_type(), name);
    if (val) builder.CreateStore(val, ptr, false);
    
    variable_symbol_table::entry_type entry { ptr, type };
    state->variables.set(name, entry);    
    return ptr;
  };
  
  return errors::codegen_call_args(op, init_value, init_type);
}

/** Global Variable Declaration **/

raytrace::ast::global_variable_decl::global_variable_decl(parser_state *st,
							  const string &name, const type_spec &type) :
  global_declaration(st),
  name(name), type(type)
{

}

raytrace::codegen_value raytrace::ast::global_variable_decl::codegen(llvm::Module *module, llvm::IRBuilder <> &builder) {
  if (state->variables.has_local(name)) {
    stringstream err;
    err << "Redeclaration of variable '" << name << "'.";
    return compile_error(err.str());
  }
  
  GlobalVariable *gv = new GlobalVariable(type->llvm_type(), NULL, GlobalValue::ExternalLinkage, NULL, name);
  module->getGlobalList().push_back(gv);

  variable_symbol_table::entry_type entry { gv, type };
  state->variables.set(name, entry);
  return gv;
}


/** Variable Reference **/

raytrace::ast::variable_ref::variable_ref(parser_state *st, const lvalue_ptr &lval) :
  expression(st), lval_ref(lval)
{
  
}

codegen_value raytrace::ast::variable_ref::codegen(Module *module, IRBuilder<> &builder) {
  boost::function<codegen_value (Value *&)> op = [&builder] (Value *& val) -> codegen_value {
    return builder.CreateLoad(val, "var_ref");
  };
  
  raytrace::errors::value_container_operation<codegen_value> create_load(op);
  
  codegen_value addr = lval_ref->codegen(module, builder);
  return errors::codegen_call(addr, op);
}

codegen_value raytrace::ast::variable_ref::codegen_ptr(Module *module, IRBuilder<> &builder) {
  return lval_ref->codegen(module, builder);
}

raytrace::type_spec raytrace::ast::variable_ref::typecheck() {
  return lval_ref->typecheck();
}

/** Variable L-Value **/

raytrace::ast::variable_lvalue::variable_lvalue(parser_state *st, const string &name) :
  lvalue(st, st->types["void"]), name(name)
{
  
}

codegen_value raytrace::ast::variable_lvalue::codegen(Module *module, IRBuilder<> &builder) {
  try {
    variable_symbol_table::entry_type &entry = state->variables.get(name);
    return entry.value;
  }
  catch (compile_error &e) { return e; }
}

raytrace::type_spec raytrace::ast::variable_lvalue::typecheck() {
  return state->variables.get(name).type;
}

/** Assignment **/

raytrace::ast::assignment::assignment(parser_state *st, const lvalue_ptr &lhs, const expression_ptr &rhs) :
  expression(st), lhs(lhs), rhs(rhs)
{
  
}

codegen_value raytrace::ast::assignment::codegen(Module *module, IRBuilder<> &builder) {
  typedef raytrace::errors::argument_value_join<codegen_value, codegen_value, typecheck_value>::result_value_type arg_val_type;  
  boost::function<codegen_value (arg_val_type &)> op = [module, &builder] (arg_val_type &args) -> codegen_value {
    try {
      type_spec t = args.get<2>();
      Value *copied = t->copy(args.get<0>(), module, builder);
      builder.CreateStore(copied, args.get<1>(), false);
      return copied;
    }
    catch (compile_error &e) { return e; }
  };

  typecheck_value t = typecheck_safe();  
  codegen_value ptr = lhs->codegen(module, builder);
  codegen_value value = rhs->codegen(module, builder);
  
  return errors::codegen_call_args(op, value, ptr, t);
}

raytrace::type_spec ast::assignment::typecheck() {
  type_spec lt = lhs->typecheck();
  type_spec rt = rhs->typecheck();
  
  if (lt != rt) {
    stringstream err;
    err << "Cannot assign value of type '" << rt->name << "' to variable of type '" << lt->name << "'.";
    throw compile_error(err.str());
  }

  return lt;
}

/** Type Constructors **/

raytrace::ast::type_constructor::type_constructor(parser_state *st, const type_spec &type, const vector<expression_ptr> &args) :
  expression(st), type(type), args(args)
{

}

codegen_value raytrace::ast::type_constructor::codegen(Module *module, IRBuilder<> &builder) {
  typed_value_vector arg_values;
  for (unsigned int i = 0; i < args.size(); i++) {
    typecheck_value t = get_argtype(i);
    codegen_value v = get_argval(i, module, builder);
    typed_value_container val = errors::combine_arg_list(v, t);
    arg_values = errors::codegen_vector_push_back(arg_values, val);
  }

  return type->create(module, builder, arg_values);
}

codegen_value raytrace::ast::type_constructor::get_argval(int i, Module *module, IRBuilder<> &builder) {
  return args[i]->codegen(module, builder);
}

typecheck_value ast::type_constructor::get_argtype(int i) {
  return args[i]->typecheck_safe();
}
