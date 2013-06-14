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
      variables().set(it->name, entry);
    }
  };
  
  //enter new scope
  push_distribution_context(name, param_type->getPointerTo(), loader);
  
  //evaluate all internal declarations
  codegen_vector content_eval;
  for (auto it = internal_decl.begin(); it != internal_decl.end(); it++) {
    codegen_value eval = (*it)->codegen(module, builder);
    content_eval = errors::codegen_vector_push_back(content_eval, eval);
  }

  //find the 'evaluate' function (error if not declared)
  function_key eval_key;
  eval_key.name = "evaluate";
  eval_key.arguments.push_back(state->types["vec3"]); /* P_in */
  eval_key.arguments.push_back(state->types["vec3"]); /* w_in */
  eval_key.arguments.push_back(state->types["vec3"]); /* P_out */
  eval_key.arguments.push_back(state->types["vec3"]);  /* w_out */
  eval_key.arguments.push_back(state->types["float"]);  /* out pdf */
  
  codegen_value eval_func = nullptr;
  if (functions().has_local(eval_key)) {
    function_entry &entry = functions().get(eval_key);
    eval_func = entry.func;

    if (!entry.arguments.back().output) {
      eval_func = errors::make_error<errors::error_message>("Distribution 'evaluate' function must have an output argument.", line_no, column_no);
    }
  }
  else eval_func = errors::make_error<errors::error_message>("Distribution must define an 'evaluate' function.", line_no, column_no);

  //find the 'sample' function
  function_key sample_key;
  sample_key.name = "sample";
  sample_key.arguments.push_back(state->types["vec3"]); /* P_out */
  sample_key.arguments.push_back(state->types["vec3"]); /* w_out */
  sample_key.arguments.push_back(state->types["vec2"]); /* rand_P */
  sample_key.arguments.push_back(state->types["vec2"]); /* rand_w */
  sample_key.arguments.push_back(state->types["vec3"]); /* out P_in */
  sample_key.arguments.push_back(state->types["vec3"]); /* out w_in */

  codegen_value sample_func = nullptr;
  if (functions().has_local(sample_key)) {
    function_entry &entry = functions().get(sample_key);
    sample_func = entry.func;
    
    //make sure the last two arguments are outputs
    if (!(entry.arguments[4].output && entry.arguments[5].output)) {
      sample_func = errors::make_error<errors::error_message>("Distribution 'sample' function must provide 2 output arguments.", line_no, column_no);
    }
  }
  else sample_func = errors::make_error<errors::error_message>("Distribution must define a 'sample' function.", line_no, column_no);

  //leave new scope
  pop_distribution_context(module, builder);
  
  //define an externally visible function to evaluate this distribution (vectors must be passed by pointer, not value).
  //then define an externally visible function to instantiate this distribution
  typedef errors::argument_value_join<codegen_value, codegen_value>::result_value_type dist_functions_type;
  boost::function<codegen_value (dist_functions_type &)> check_funcs = [this, module, &builder] (dist_functions_type &args) -> codegen_value {
    Function *eval_f = createEvaluator(cast<Function>(args.get<0>()), module, builder);
    Function *sample_f = createSampler(cast<Function>(args.get<1>()), module, builder);
    Function *dtor_f = createDestructor(module, builder);
    
    return createConstructor(module, builder, eval_f, sample_f, dtor_f);
  };
  codegen_value ctor_val = errors::codegen_call_args(check_funcs, eval_func, sample_func);
  
  //final error checking, add the constructor to the function symtab
  typedef errors::argument_value_join<codegen_vector, codegen_value, codegen_value>::result_value_type arg_val_type;
  boost::function<codegen_value (arg_val_type &)> op = [this, module, &builder] (arg_val_type &args) -> codegen_value {
    Function *ctor = cast<Function>(args.get<2>());
    function_entry entry = function_entry::make_entry(name, function_scope_name(),
						      state->types["dfunc"], params);
    entry.func = ctor;
    function_table().set(entry.to_key(), entry);

    //also export the constructor
    exports::function_export exp;
    exp.name = entry.name;
    exp.full_name = entry.full_name;
    exp.return_type = entry.return_type;
    exp.arguments = entry.arguments;
    state->exports.add_function(exp);

    return ctor;
  };
  
  return errors::codegen_call_args(op, content_eval, eval_func, ctor_val);
}

Function *ast::distribution::createConstructor(Module *module, IRBuilder<> &builder,
					       Function *eval, Function *sample, Function *dtor) {
  string ctor_name = function_generate_name(name, function_scope_name(), params);
  
  //create function accepting parameters as arguments
  vector<Type*> arg_types;
  for (auto it = params.begin(); it != params.end(); ++it) arg_types.push_back(it->type->llvm_type());
  FunctionType *ft = FunctionType::get(state->types["dfunc"]->llvm_type(), arg_types, false);
  Function *f = Function::Create(ft, Function::ExternalLinkage, ctor_name, module);
  BasicBlock *bb = BasicBlock::Create(getGlobalContext(), "func_entry", f);
  builder.SetInsertPoint(bb);
  
  //setup arguments for the alloc call
  Value *gd_scene = module->getNamedGlobal(".__gd_scene");
  assert(gd_scene != NULL);
  Value *scene_ptr = builder.CreateLoad(gd_scene);

  //get memory for a new distribution object
  Value *dfunc_ptr = state->types["dfunc"]->allocate(module, builder);
  
  //initialize the object and dynamically allocate parameter memory (calling a builtin function)
  vector<Type*> alloc_arg_types({state->types["scene_ptr"]->llvm_type(), Type::getInt32Ty(getGlobalContext()),
	eval->getType(), sample->getType(), dtor->getType(), dfunc_ptr->getType()});
  FunctionType *alloc_type = FunctionType::get(Type::getInt32PtrTy(getGlobalContext()), alloc_arg_types, false);
  Function *alloc_func = cast<Function>(module->getOrInsertFunction("gd_builtin_alloc_dfunc", alloc_type));
  
  int param_data_size = DataLayout(module).getTypeAllocSize(param_type);
  Constant *param_size_arg = ConstantInt::get(getGlobalContext(), APInt(8*sizeof(int), param_data_size));

  vector<Value*> alloc_args({scene_ptr, param_size_arg, eval, sample, dtor, dfunc_ptr});
  Value *param_ptr = builder.CreatePointerCast(builder.CreateCall(alloc_func, alloc_args),
					       param_type->getPointerTo(), "dfunc_param_ptr");

  //set each parameter
  auto arg_it = f->arg_begin();
  unsigned int field_idx = 0;
  for (auto it = params.begin(); it != params.end(); ++it, ++arg_it, ++field_idx) {
    Value *param_copy = it->type->copy(arg_it, module, builder);
    it->type->store(param_copy, builder.CreateStructGEP(param_ptr, field_idx), module, builder);
  }
  
  //return the object
  Value *rt_val = builder.CreateLoad(dfunc_ptr, "dist_ref");
  builder.CreateRet(rt_val);
  return f;
}

Function *ast::distribution::createDestructor(Module *module, IRBuilder<> &builder) {
  string dtor_name = evaluator_name("dtor");
  
  Type *param_ptr_ty = Type::getInt32PtrTy(getGlobalContext());
  FunctionType *dtor_type = FunctionType::get(Type::getVoidTy(getGlobalContext()),
					      ArrayRef<Type*>(vector<Type*>(1, param_ptr_ty)), false);
  Function *dtor = Function::Create(dtor_type, Function::ExternalLinkage, dtor_name, module);
  BasicBlock *bb = BasicBlock::Create(getGlobalContext(), "func_entry", dtor);
  builder.SetInsertPoint(bb);

  Value *param_ptr = builder.CreatePointerCast(dtor->arg_begin(), param_type->getPointerTo(), "param_struct_ptr");
  for (unsigned int i = 0;  i < params.size(); ++i) {
    Value *ptr = builder.CreateStructGEP(param_ptr, i);
    type_spec type = params[i].type;
    type->destroy(ptr, module, builder);
  }

  builder.CreateRetVoid();

  return dtor;
}

string ast::distribution::evaluator_name(const string &n) {
  stringstream name_ss;
  name_ss << "gd_dist." << function_scope_name() << "." << name << "." << n;
  return name_ss.str();
}

Function *ast::distribution::createEvaluator(Function *eval, Module *module, IRBuilder<> &builder) {
  Type *vec3_ptr = state->types["vec3"]->llvm_type()->getPointerTo();
  Type *vec4_ptr = state->types["vec4"]->llvm_type()->getPointerTo();
  Type *float_ptr = state->types["float"]->llvm_ptr_type();

  const unsigned int num_args = 4;
  Type *int_ptr_ty = Type::getInt32PtrTy(getGlobalContext());
  Type *param_ptr_ty = param_type->getPointerTo();

  vector<Type*> arg_types({int_ptr_ty, vec3_ptr, vec3_ptr, vec3_ptr, vec3_ptr, /* out */ float_ptr, /* out */ vec4_ptr});
  FunctionType *eval_type = FunctionType::get(Type::getVoidTy(getGlobalContext()), arg_types, false);
  Function *f = Function::Create(eval_type, Function::ExternalLinkage, evaluator_name("eval"), module);
  
  BasicBlock *bb = BasicBlock::Create(getGlobalContext(), "func_entry", f);
  builder.SetInsertPoint(bb);

  vector<Value*> eval_args;

  auto arg_it = f->arg_begin();
  eval_args.push_back(builder.CreatePointerCast(arg_it, param_ptr_ty)); //just copy the first pointer value
  ++arg_it;
  
  //load all except the result argument
  for (unsigned int i = 0; i < num_args; ++i) {
    eval_args.push_back(builder.CreateLoad(arg_it));
    
    ++arg_it;
  }

  //add the final pdf argument
  eval_args.push_back(arg_it++);

  Value *rt_v4 = builder.CreateCall(eval, ArrayRef<Value*>(eval_args), "dist_eval");
  builder.CreateStore(rt_v4, arg_it);
  builder.CreateRetVoid();

  return f;
}

Function *ast::distribution::createSampler(Function *eval, Module *module, IRBuilder<> &builder) {
  Type *vec3_ptr = state->types["vec3"]->llvm_ptr_type();
  Type *vec2_ptr = state->types["vec2"]->llvm_ptr_type();

  const unsigned int num_args = 4;
  Type *int_ptr_ty = Type::getInt32PtrTy(getGlobalContext());
  Type *param_ptr_ty = param_type->getPointerTo();

  vector<Type*> arg_types({int_ptr_ty, vec3_ptr, vec3_ptr, vec2_ptr, vec2_ptr, /* out */ vec3_ptr, /* out */ vec3_ptr});
  FunctionType *sample_type = FunctionType::get(Type::getFloatTy(getGlobalContext()), arg_types, false);
  Function *f = Function::Create(sample_type, Function::ExternalLinkage, evaluator_name("sample"), module);
  
  BasicBlock *bb = BasicBlock::Create(getGlobalContext(), "func_entry", f);
  builder.SetInsertPoint(bb);

  vector<Value*> sample_args;

  auto arg_it = f->arg_begin();
  sample_args.push_back(builder.CreatePointerCast(arg_it, param_ptr_ty)); //just copy the first pointer value
  ++arg_it;
  
  //load all except the result argument
  for (unsigned int i = 0; i < num_args; ++i) {
    sample_args.push_back(builder.CreateLoad(arg_it));
    ++arg_it;
  }

  //push back the output arguments
  sample_args.push_back(arg_it++);
  sample_args.push_back(arg_it++);

  builder.CreateRet(builder.CreateCall(eval, ArrayRef<Value*>(sample_args), "dist_sample"));
  return f;
}
