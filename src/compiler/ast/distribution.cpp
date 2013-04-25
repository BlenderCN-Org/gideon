#include "compiler/ast/distribution.hpp"
#include "compiler/ast/function.hpp"
#include "compiler/llvm_helper.hpp"

using namespace raytrace;
using namespace std;
using namespace llvm;

ast::distribution::distribution(parser_state *st, const string &name,
				const vector<function_argument> &params,
				const vector<global_declaration_ptr> &internal_decl,
				unsigned int line_no, unsigned int column_no) :
  global_declaration(st),
  name(name), params(params),
  internal_decl(internal_decl),
  param_type(getParameterType())
{
  
}

StructType *ast::distribution::getParameterType() {
  //define a struct type containing all parameters
  vector<Type*> param_types;
  boost::unordered_map<string, int> name_idx_map;

  for (auto it = params.begin(); it != params.end(); ++it) {
    param_types.push_back(it->type->llvm_type());
  }

  string pstruct_name = name + string("_params_t");
  return StructType::create(getGlobalContext(), param_types, pstruct_name);
}

codegen_value ast::distribution::codegen(Module *module, IRBuilder<> &builder) {
  boost::function<void (Value*, Module*, IRBuilder<>&)> loader = [this] (Value *ctx, Module *module, IRBuilder<> &builder) -> void {
    //create variable names pointing to each member of the param struct    
    unsigned int param_idx = 0;
    for (auto it = params.begin(); it != params.end(); ++it, ++param_idx) {
      Value *param_ptr = builder.CreateStructGEP(ctx, param_idx);
      variable_symbol_table::entry_type entry(param_ptr, it->type, false);
      state->variables.set(it->name, entry);
    }
  };
  
  //enter new scope  
  state->control.push_context(NULL, param_type->getPointerTo(), loader);
  state->functions.scope_push(name);
  state->control.push_scope();
  
  //evaluate all internal declarations
  codegen_vector content_eval;
  for (auto it = internal_decl.begin(); it != internal_decl.end(); it++) {
    codegen_value eval = (*it)->codegen(module, builder);
    content_eval = errors::codegen_vector_push_back(content_eval, eval);
  }

  //find the 'evaluate' function (error if not declared)
  function_key eval_key;
  eval_key.name = "evaluate";
  eval_key.arguments.push_back(state->types["vec3"]); /* N */
  eval_key.arguments.push_back(state->types["vec3"]); /* P_in */
  eval_key.arguments.push_back(state->types["vec3"]); /* w_in */
  eval_key.arguments.push_back(state->types["vec3"]); /* P_out */
  eval_key.arguments.push_back(state->types["vec3"]);  /* w_out */
  
  codegen_value eval_func = nullptr;
  if (state->functions.has_local(eval_key)) eval_func = state->functions.get(eval_key).func;
  else eval_func = compile_error("Distribution must define an 'evaluate' function.");

  //leave new scope
  state->control.pop_scope();
  codegen_void fpop = state->functions.scope_pop(module, builder);
  state->control.pop_context();
  
  //define an externally visible function to evaluate this distribution (vectors must be passed by pointer, not value).
  //then define an externally visible function to instantiate this distribution
  codegen_value ctor_val = errors::codegen_call(eval_func, [this, module, &builder] (Value *f_val) -> codegen_value {
      Function *eval_f = createEvaluator(cast<Function>(f_val), module, builder);
      return createConstructor(module, builder, eval_f);
    });
  
  //final error checking, add the constructor to the function symtab
  typedef errors::argument_value_join<codegen_vector, codegen_value, codegen_void, codegen_value>::result_value_type arg_val_type;
  boost::function<codegen_value (arg_val_type &)> op = [this, module, &builder] (arg_val_type &args) -> codegen_value {
    Function *ctor = cast<Function>(args.get<3>());
    function_entry entry = function_entry::make_entry(name, state->functions.scope_name(),
						      state->types["dfunc"], params);
    entry.func = ctor;
    state->functions.set(entry.to_key(), entry);
    return ctor;
  };
  
  return errors::codegen_call_args(op, content_eval, eval_func, fpop, ctor_val);
}

Function *ast::distribution::createConstructor(Module *module, IRBuilder<> &builder,
					       Function *eval) {
  string ctor_name = function_generate_name(name, state->functions.scope_name(), params);
  
  //create function accepting parameters as arguments
  vector<Type*> arg_types;
  for (auto it = params.begin(); it != params.end(); ++it) arg_types.push_back(it->type->llvm_type());
  FunctionType *ft = FunctionType::get(state->types["dfunc"]->llvm_type(), arg_types, false);
  Function *f = Function::Create(ft, Function::ExternalLinkage, ctor_name, module);
  BasicBlock *bb = BasicBlock::Create(getGlobalContext(), "func_entry", f);
  builder.SetInsertPoint(bb);
  
  //setup arguments for the alloc call
  variable_symbol_table::entry_type scene_var = state->variables.get("__gd_scene");
  Value *scene_ptr = builder.CreateLoad(scene_var.value);

  //get memory for a new distribution object
  Value *dfunc_ptr = CreateEntryBlockAlloca(builder, state->types["dfunc"]->llvm_type(), "new_dist");
  
  //initialize the object and dynamically allocate parameter memory (calling a builtin function)
  vector<Type*> alloc_arg_types({state->types["scene_ptr"]->llvm_type(), Type::getInt32Ty(getGlobalContext()),
	eval->getType(), dfunc_ptr->getType()});
  FunctionType *alloc_type = FunctionType::get(param_type->getPointerTo(), alloc_arg_types, false);
  Function *alloc_func = cast<Function>(module->getOrInsertFunction("gd_builtin_alloc_dfunc", alloc_type));
  
  int param_data_size = DataLayout(module).getTypeAllocSize(param_type);
  Constant *param_size_arg = ConstantInt::get(getGlobalContext(), APInt(8*sizeof(int), param_data_size));
  Value *param_ptr = builder.CreateCall4(alloc_func, scene_ptr, param_size_arg, eval, dfunc_ptr);

  //set each parameter
  auto arg_it = f->arg_begin();
  unsigned int field_idx = 0;
  for (auto it = params.begin(); it != params.end(); ++it, ++arg_it, ++field_idx) {
    Value *param_copy = it->type->copy(arg_it, module, builder);
    builder.CreateStore(param_copy, builder.CreateStructGEP(param_ptr, field_idx));
  }
  
  //return the object
  Value *rt_val = builder.CreateLoad(dfunc_ptr, "dist_ref");
  builder.CreateRet(rt_val);
  return f;
}

string ast::distribution::evaluator_name() const {
  stringstream name_ss;
  name_ss << "gd_dist_" << state->functions.scope_name() << "_" << name;
  return name_ss.str();
}

Function *ast::distribution::createEvaluator(Function *eval, Module *module, IRBuilder<> &builder) {
  Type *vec3_ptr = state->types["vec3"]->llvm_type()->getPointerTo();
  Type *vec4_ptr = state->types["vec4"]->llvm_type()->getPointerTo();

  const unsigned int num_args = 5;
  vector<Type*> arg_types({param_type->getPointerTo(), vec3_ptr, vec3_ptr, vec3_ptr, vec3_ptr, vec3_ptr, /* out */ vec4_ptr});
  FunctionType *eval_type = FunctionType::get(Type::getVoidTy(getGlobalContext()), arg_types, false);
  Function *f = Function::Create(eval_type, Function::ExternalLinkage, evaluator_name(), module);
  
  BasicBlock *bb = BasicBlock::Create(getGlobalContext(), "func_entry", f);
  builder.SetInsertPoint(bb);

  vector<Value*> eval_args;

  auto arg_it = f->arg_begin();
  eval_args.push_back(arg_it); //just copy the first pointer value
  ++arg_it;
  
  //load all except the result argument
  for (unsigned int i = 0; i < num_args; ++i) {
    eval_args.push_back(builder.CreateLoad(arg_it));
    ++arg_it;
  }

  Value *rt_v4 = builder.CreateCall(eval, ArrayRef<Value*>(eval_args), "dist_eval");
  builder.CreateStore(rt_v4, arg_it);
  builder.CreateRetVoid();

  return f;
}
