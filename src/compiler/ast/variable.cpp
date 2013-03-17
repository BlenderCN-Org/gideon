#include "compiler/ast/variable.hpp"

#include <stdexcept>
#include <sstream>

using namespace std;
using namespace raytrace::ast;
using namespace llvm;

/** Variable Declaration **/

raytrace::ast::variable_decl::variable_decl(var_symbol_table *symtab, const string &name,
					    const type_spec &type, const expression_ptr &init) :
  statement(),
  symtab(symtab), name(name), type(type), initializer(init)
{
  
}

Value *variable_decl::codegen(Module *module, IRBuilder<> &builder) {
  if (symtab->has_local_name(name)) {
    stringstream err;
    err << "Redeclaration of variable '" << name << "'.";
    throw runtime_error(err.str());
  }
  
  Value *ptr = NULL;
  ptr = builder.CreateAlloca(type.llvm_type(), NULL, name.c_str());

  if (initializer) {
    Value *init_value = initializer->codegen(module, builder);
    builder.CreateStore(init_value, ptr, false);
  }
  
  var_symbol_table::table_entry entry { ptr, type };
  symtab->set(name, entry);
  
  return ptr;
}

/** Variable Reference **/

raytrace::ast::variable_ref::variable_ref(const lvalue_ptr &lval) :
  expression(), lval_ref(lval)
{
  
}

Value *raytrace::ast::variable_ref::codegen(Module *module, IRBuilder<> &builder) {
  Value *addr = lval_ref->codegen(module, builder);
  return builder.CreateLoad(addr, "var_ref");
}

Value *raytrace::ast::variable_ref::codegen_ptr(Module *module, IRBuilder<> &builder) {
  return lval_ref->codegen(module, builder);
}

raytrace::type_spec raytrace::ast::variable_ref::typecheck() {
  return lval_ref->typecheck();
}

/** Variable L-Value **/

raytrace::ast::variable_lvalue::variable_lvalue(var_symbol_table *symtab, const string &name) :
  lvalue(type_code::OTHER), name(name), symtab(symtab)
{
  
}

Value *raytrace::ast::variable_lvalue::codegen(Module *, IRBuilder<> &builder) {
  var_symbol_table::table_entry &entry = symtab->get(name);
  return entry.value;
}

raytrace::type_spec raytrace::ast::variable_lvalue::typecheck() {
  return symtab->get(name).type;
}

/** Assignment **/

raytrace::ast::assignment::assignment(const lvalue_ptr &lhs, const expression_ptr &rhs) :
  expression(), lhs(lhs), rhs(rhs)
{
  
}

Value *raytrace::ast::assignment::codegen(Module *module, IRBuilder<> &builder) {
  typecheck();
  
  Value *ptr = lhs->codegen(module, builder);
  Value *value = rhs->codegen(module, builder);
  builder.CreateStore(value, ptr, false);

  return value;
}

raytrace::type_spec assignment::typecheck() {
  type_spec lt = lhs->typecheck();
  type_spec rt = rhs->typecheck();
  
  if (lt != rt) {
    type_traits lt_t = get_type_traits(lt.t), rt_t = get_type_traits(rt.t);
    stringstream err;
    err << "Cannot assign value of type '" << rt_t.name << "' to variable of type '" << lt_t.name << "'.";
    throw runtime_error(err.str());
  }

  return lt;
}

/** Type Constructors **/

raytrace::ast::type_constructor::type_constructor(const type_spec &type, const vector<expression_ptr> &args) :
  type(type), args(args)
{

}

Value *raytrace::ast::type_constructor::codegen(Module *module, IRBuilder<> &builder) {
  if (type.t == FLOAT4) {
    type_spec val_type = {type_code::FLOAT};
    check_args({val_type, val_type, val_type, val_type});
    
    return make_llvm_float4(module, builder,
			    get_argval(0, module, builder), get_argval(1, module, builder),
			    get_argval(2, module, builder), get_argval(3, module, builder));
  }
}

Value *raytrace::ast::type_constructor::get_argval(int i, Module *module, IRBuilder<> &builder) {
  return args[0]->codegen(module, builder);
}

void raytrace::ast::type_constructor::check_args(const vector<type_spec> &expected) {
  string type_name = get_type_traits(type.t).name;
  stringstream err_str;
  err_str << "Constructor error for type '" << type_name << "': ";

  if (args.size() != expected.size()) {
    err_str << "Found " << args.size() << " arguments, expected " << expected.size();
    throw runtime_error(err_str.str());
  }

  for (unsigned int idx = 0; idx < args.size(); idx++) {
    type_spec arg_type = args[idx]->typecheck();
    if (arg_type != expected[idx]) {
      type_traits arg_tt = get_type_traits(arg_type.t);
      type_traits exp_tt = get_type_traits(expected[idx].t);
      
      err_str << "Invalid type for argument " << idx + 1 << ": Found '" << arg_tt.name << "' but expected '" << exp_tt.name << "'";
      throw runtime_error(err_str.str());
    }
  }
}
