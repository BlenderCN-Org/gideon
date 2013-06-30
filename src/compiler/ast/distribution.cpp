#include "compiler/ast/distribution.hpp"
#include "compiler/ast/function.hpp"
#include "compiler/llvm_helper.hpp"

using namespace raytrace;
using namespace std;
using namespace llvm;

ast::distribution::distribution(parser_state *st, const string &name,
				const vector<expression_ptr> &flags,
				const vector<distribution_parameter> &params,
				const vector<global_declaration_ptr> &internal_decl,
				unsigned int line_no, unsigned int column_no) :
  global_declaration(st),
  name(name), flags(flags), params(params),
  internal_decl(internal_decl)
{
  
}

typecheck_vector ast::distribution::get_parameter_types() {
  typecheck_vector param_type_list;
  for (auto p_it : params) {
    typecheck_value p_ty = p_it.type->codegen_type();
    param_type_list = errors::codegen_vector_push_back(param_type_list, p_ty);
  }

  return param_type_list;
}

StructType *ast::distribution::getParameterType(const std::vector<type_spec> &param_types) {
  //define a struct type containing all parameters
  vector<Type*> struct_params;
  boost::unordered_map<string, int> name_idx_map;

  for (auto it = param_types.begin(); it != param_types.end(); ++it) {
    struct_params.push_back((*it)->llvm_type());
  }

  string pstruct_name = name + string("_params_t");
  return StructType::create(getGlobalContext(), struct_params, pstruct_name);
}

codegen_value ast::distribution::codegen(Module *module, IRBuilder<> &builder) {
  typecheck_vector param_types = get_parameter_types();

  return errors::codegen_call<typecheck_vector, codegen_value>(param_types, [this, module, &builder] (vector<type_spec> &param_types) -> codegen_value {
      Type *param_struct_ty = getParameterType(param_types);

      boost::function<void (Value*, Module*, IRBuilder<>&)> loader = [this, &param_types] (Value *ctx, Module *module, IRBuilder<> &builder) -> void {
	//create variable names pointing to each member of the param struct    
	for (unsigned int param_idx = 0; param_idx < params.size(); ++param_idx) {
	  const distribution_parameter &param = params[param_idx];
	  
	  Value *param_ptr = builder.CreateStructGEP(ctx, param_idx);
	  variable_symbol_table::entry_type entry(param_ptr, param_types[param_idx], false);
	  variables().set(param.name, entry);
	}
      };
      
      //enter new scope
      push_distribution_context(name, param_struct_ty->getPointerTo(), loader);
      
      //evaluate all internal declarations
      codegen_vector content_eval;
      for (auto it = internal_decl.begin(); it != internal_decl.end(); it++) {
	codegen_value eval = (*it)->codegen(module, builder);
	content_eval = errors::codegen_vector_push_back(content_eval, eval);
      }
      
      //find the 'evaluate' function (error if not declared)
      codegen_value eval_func = check_for_evaluate();

      //find the 'pdf' function
      codegen_value pdf_func = check_for_pdf();

      //find the 'emission' function
      codegen_value emit_func = check_for_emission();
      
      //find the 'sample' function
      codegen_value sample_func = check_for_sample();
      
      //leave new scope
      pop_distribution_context(module, builder);
      
      vector<function_argument> ctor_arguments;
      for (unsigned int param_idx = 0; param_idx < params.size(); ++param_idx) {
	function_argument arg{params[param_idx].name, param_types[param_idx], false};
	ctor_arguments.push_back(arg);
      }

      string ctor_name = function_generate_name(name, function_scope_name(), ctor_arguments);

      //define an externally visible function to evaluate this distribution (vectors must be passed by pointer, not value).
      //then define an externally visible function to instantiate this distribution
      boost::function<codegen_value (Value *&, Value *&,
				     Value *&, Value *&)> check_funcs = [this,
									 &ctor_name,
									 &param_types, param_struct_ty,
									 module, &builder] (Value *&eval_ptr,
											    Value *&sample_ptr,
											    Value *&pdf_ptr,
											    Value *&emit_ptr) -> codegen_value {
	Value *eval_f = create_evaluator(cast_or_null<Function>(eval_ptr), param_struct_ty, module, builder);
	Value *sample_f = create_sampler(cast_or_null<Function>(sample_ptr), param_struct_ty, module, builder);
	Value *pdf_f = create_pdf(cast_or_null<Function>(pdf_ptr), param_struct_ty, module, builder);
	Value *emit_f = create_emission(cast_or_null<Function>(emit_ptr), param_struct_ty, module, builder);
	
	Function *dtor_f = createDestructor(module, builder, param_struct_ty, param_types);
	
	return createConstructor(module, builder,
				 ctor_name,
				 param_struct_ty, param_types,
				 eval_f, sample_f, pdf_f, emit_f,
				 dtor_f);
      };
      codegen_value ctor_val = errors::codegen_apply(check_funcs, eval_func, sample_func, pdf_func, emit_func);
      
      //final error checking, add the constructor to the function symtab
      boost::function<codegen_value (vector<Value*> &, Value*&, Value*&)> op = [this,
										&ctor_arguments,
										&param_types,
										module, &builder] (vector<Value*> &,
												   Value *&,
												   Value *&ctor_ptr) -> codegen_value {

	Function *ctor = cast<Function>(ctor_ptr);
	function_entry entry = function_entry::make_entry(name, function_scope_name(),
							  state->types["dfunc"], ctor_arguments);
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
      
      return errors::codegen_apply(op, content_eval, eval_func, ctor_val);
    });
}

codegen_value ast::distribution::check_for_function(const function_key &key,
						    const vector<bool> &output_args,
						    bool use_default) {
  codegen_value func = nullptr;
  if (functions().has_local(key)) {
    function_entry &entry = functions().get(key);
    func = entry.func;
    
    unsigned int arg_idx = 0;
    for (auto it = entry.arguments.begin(); it != entry.arguments.end(); ++it, ++arg_idx) {
      if ((arg_idx < output_args.size()) &&
	  output_args[arg_idx] && !it->output) {
	stringstream err_ss;
	err_ss << "Distribution '" << key.name << "' function expects an output argument.";
	func = errors::make_error<errors::error_message>(err_ss.str(), line_no, column_no);
      }
    }
  }
  else {
    if (use_default) {
      return nullptr;
    }
    else {
      stringstream err_ss;
      err_ss << "Distribution must define an '" << key.name << "' function.";
      func = errors::make_error<errors::error_message>(err_ss.str(), line_no, column_no);
    }
  }

  return func;
}



codegen_value ast::distribution::createConstructor(Module *module, IRBuilder<> &builder,
						   const string &ctor_name,
						   Type *parameter_type,
						   const vector<type_spec> &param_type_list,
						   Value *eval, Value *sample, Value *pdf, Value *emit,
						   Function *dtor) {
  //create function accepting parameters as arguments
  vector<Type*> arg_types;
  for (auto it = param_type_list.begin(); it != param_type_list.end(); ++it) arg_types.push_back((*it)->llvm_type());

  FunctionType *ft = FunctionType::get(state->types["dfunc"]->llvm_type(), arg_types, false);
  Function *f = Function::Create(ft, Function::ExternalLinkage, ctor_name, module);
  BasicBlock *bb = BasicBlock::Create(getGlobalContext(), "func_entry", f);
  builder.SetInsertPoint(bb);
  
  //setup arguments for the alloc call
  Value *gd_scene = module->getNamedGlobal(".__gd_scene");
  assert(gd_scene != NULL);
  Value *scene_ptr = builder.CreateLoad(gd_scene);

  //compute the shader flags
  codegen_value flag_val = codegen_all_flags(module, builder);

  return errors::codegen_call(flag_val, [&] (Value *&flag_bitmask) -> codegen_value {
      //get memory for a new distribution object
      Value *dfunc_ptr = state->types["dfunc"]->allocate(module, builder);
  
      //initialize the object and dynamically allocate parameter memory (calling a builtin function)
      Type* int_ptr_ty = Type::getInt32Ty(getGlobalContext())->getPointerTo();
      vector<Type*> alloc_arg_types({state->types["scene_ptr"]->llvm_type(),
	    Type::getInt32Ty(getGlobalContext()), state->types["shader_flag"]->llvm_type(),
	    int_ptr_ty, int_ptr_ty, int_ptr_ty, int_ptr_ty,
	    dtor->getType(), dfunc_ptr->getType()});
      FunctionType *alloc_type = FunctionType::get(Type::getInt32PtrTy(getGlobalContext()), alloc_arg_types, false);
      Function *alloc_func = cast<Function>(module->getOrInsertFunction("gd_builtin_alloc_dfunc", alloc_type));
      
      int param_data_size = DataLayout(module).getTypeAllocSize(parameter_type);
      Constant *param_size_arg = ConstantInt::get(getGlobalContext(), APInt(8*sizeof(int), param_data_size));
      
      vector<Value*> alloc_args({scene_ptr, param_size_arg, flag_bitmask,
	    builder.CreatePointerCast(eval, int_ptr_ty),
	    builder.CreatePointerCast(sample, int_ptr_ty),
	    builder.CreatePointerCast(pdf, int_ptr_ty),
	    builder.CreatePointerCast(emit, int_ptr_ty),
	    dtor, dfunc_ptr});
      Value *param_ptr = builder.CreatePointerCast(builder.CreateCall(alloc_func, alloc_args),
						   parameter_type->getPointerTo(), "dfunc_param_ptr");
      
      //set each parameter
      auto arg_it = f->arg_begin();
      unsigned int field_idx = 0;
      for (auto it = param_type_list.begin(); it != param_type_list.end(); ++it, ++arg_it, ++field_idx) {
	Value *param_copy = (*it)->copy(arg_it, module, builder);
	(*it)->store(param_copy, builder.CreateStructGEP(param_ptr, field_idx), module, builder);
      }
      
      //return the object
      Value *rt_val = builder.CreateLoad(dfunc_ptr, "dist_ref");
      builder.CreateRet(rt_val);
      return f;
    });
}

Function *ast::distribution::createDestructor(Module *module, IRBuilder<> &builder,
					      Type *param_ty, const vector<type_spec> &param_type_list) {
  string dtor_name = evaluator_name("dtor");
  
  Type *param_ptr_ty = Type::getInt32PtrTy(getGlobalContext());
  FunctionType *dtor_type = FunctionType::get(Type::getVoidTy(getGlobalContext()),
					      ArrayRef<Type*>(vector<Type*>(1, param_ptr_ty)), false);
  Function *dtor = Function::Create(dtor_type, Function::ExternalLinkage, dtor_name, module);
  BasicBlock *bb = BasicBlock::Create(getGlobalContext(), "func_entry", dtor);
  builder.SetInsertPoint(bb);

  Value *param_ptr = builder.CreatePointerCast(dtor->arg_begin(), param_ty->getPointerTo(), "param_struct_ptr");
  for (unsigned int i = 0;  i < param_type_list.size(); ++i) {
    Value *ptr = builder.CreateStructGEP(param_ptr, i);
    type_spec type = param_type_list[i];
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

codegen_value ast::distribution::check_for_evaluate() {
  return check_for_function({"evaluate",
	{
	  state->types["vec3"], /* P_in */
	    state->types["vec3"], /* w_in */
	    state->types["vec3"], /* P_out */
	    state->types["vec3"], /* w_out */
	    state->types["float"] /* out pdf */
	    }
    },
    {false, false, false, false, true});
}

Value *ast::distribution::create_evaluator(Function *eval, Type *parameter_type,
					   Module *module, IRBuilder<> &builder) {
  wrapper_argument v3_arg{state->types["vec3"], false};
  wrapper_argument f_arg{state->types["float"], true};
  vector<wrapper_argument> wrapped_args{v3_arg, v3_arg, v3_arg, v3_arg, f_arg};
  return create_wrapper(eval, evaluator_name("eval"), parameter_type,
			wrapped_args,
			Type::getVoidTy(getGlobalContext()),
			true, state->types["vec4"],
			module, builder);
}

codegen_value ast::distribution::check_for_sample() {
  return check_for_function({"sample",
	{
	  state->types["vec3"], /* P_out */
	    state->types["vec3"], /* w_out */
	    state->types["vec2"], /* rand_P */
	    state->types["vec2"], /* rand_w */
	    state->types["vec3"], /* out P_in */
	    state->types["vec3"] /* out w_in */
	    }},
    {false, false, false, false, true, true});
}

Value *ast::distribution::create_sampler(Function *sample, Type *parameter_type,
					 Module *module, IRBuilder<> &builder) {
  wrapper_argument v3_arg{state->types["vec3"], false};
  wrapper_argument v3_out{state->types["vec3"], true};
  wrapper_argument v2_arg{state->types["vec2"], false};
  vector<wrapper_argument> wrapped_args{v3_arg, v3_arg, v2_arg, v2_arg, v3_out, v3_out};
  return create_wrapper(sample, evaluator_name("sample"), parameter_type,
			wrapped_args,
			state->types["float"]->llvm_type(),
			false, NULL,
			module, builder);
}

codegen_value ast::distribution::check_for_pdf() {
  return check_for_function({"pdf",
	{
	  state->types["vec3"], /* P_in */
	    state->types["vec3"], /* w_in */
	    state->types["vec3"], /* P_out */
	    state->types["vec3"], /* w_out */
	    }},
    {false, false, false, false},
    true);
}

Value *ast::distribution::create_pdf(Function *pdf, Type *parameter_type,
				     Module *module, IRBuilder<> &builder) {
  wrapper_argument v3_arg{state->types["vec3"], false};
  vector<wrapper_argument> wrapped_args{v3_arg, v3_arg, v3_arg, v3_arg};
  return create_wrapper(pdf, evaluator_name("pdf"), parameter_type,
			wrapped_args,
			state->types["float"]->llvm_type(),
			false, NULL,
			module, builder);
}

codegen_value ast::distribution::check_for_emission() {
  return check_for_function({"emission",
	{
	  state->types["vec3"], /* P_out */
	    state->types["vec3"], /* w_out */
	    }},
    {false, false},
    true);
}
 
Value *ast::distribution::create_emission(Function *emit, Type *parameter_type,
					  Module *module, IRBuilder<> &builder) {
  wrapper_argument v3_arg{state->types["vec3"], false};
  vector<wrapper_argument> wrapped_args{v3_arg, v3_arg};
  return create_wrapper(emit, evaluator_name("emit"), parameter_type,
			wrapped_args,
			Type::getVoidTy(getGlobalContext()),
			true, state->types["vec4"],
			module, builder);
}

Value *ast::distribution::create_wrapper(Function *func, const string &name,
					 Type *parameter_type,
					 const vector<wrapper_argument> &arguments,
					 Type *return_type,
					 bool last_arg_as_return, const type_spec &return_arg_type,
					 Module *module, IRBuilder<> &builder) {
  if (!func) {
    //if the function to be wrapped is NULL, return a null pointer.
    return ConstantPointerNull::get(PointerType::getUnqual(FunctionType::get(Type::getVoidTy(getGlobalContext()), false)));
  }

  Type *int_ptr_ty = Type::getInt32PtrTy(getGlobalContext());
  Type *param_ptr_ty = parameter_type->getPointerTo();

  //build the argument type list
  //for non-primitive types, we need to pass a pointer.
  vector<Type*> arg_types;
  arg_types.push_back(int_ptr_ty);
  for (auto it = arguments.begin(); it != arguments.end(); ++it) {
    Type *arg_ty = it->ty->llvm_type();
    if (!arg_ty->isPrimitiveType() || it->is_output) arg_types.push_back(it->ty->llvm_ptr_type());
    else arg_types.push_back(arg_ty);
  }

  //if we're using the last argument as a return value, add it here
  if (last_arg_as_return) arg_types.push_back(return_arg_type->llvm_ptr_type());
  
  FunctionType *func_type = FunctionType::get(return_type, arg_types, false);
  Function *f = Function::Create(func_type, Function::ExternalLinkage, name, module);

  BasicBlock *bb = BasicBlock::Create(getGlobalContext(), "func_entry", f);
  builder.SetInsertPoint(bb);

  vector<Value*> unwrapped_args;

  auto arg_it = f->arg_begin();
  unwrapped_args.push_back(builder.CreatePointerCast(arg_it, param_ptr_ty)); //just copy the first pointer value
  ++arg_it;

  //for any argument that was passed by reference, load it (except output arguments)
  for (auto wrapped_it = arguments.begin(); wrapped_it != arguments.end(); ++wrapped_it, ++arg_it) {
    type_spec t = wrapped_it->ty;
    Type *arg_ty = t->llvm_type();
    bool do_load = !arg_ty->isPrimitiveType() && !wrapped_it->is_output;
    if (do_load) unwrapped_args.push_back(t->load(arg_it, module, builder));
    else unwrapped_args.push_back(arg_it);
  }
  
  //call the wrapped function
  Value *call = builder.CreateCall(func, ArrayRef<Value*>(unwrapped_args), "func_result");
  if (last_arg_as_return) {
    return_arg_type->store(call, arg_it, module, builder);
    builder.CreateRetVoid();
  }
  else builder.CreateRet(call);

  return f;
}

codegen_value ast::distribution::codegen_all_flags(Module *module, IRBuilder<> &builder) {
  typed_value_vector flag_vals;
  for (auto flag_it = flags.begin(); flag_it != flags.end(); ++flag_it) {
    typed_value_container flag = (*flag_it)->codegen(module, builder);
    flag_vals = errors::codegen_vector_push_back(flag_vals, flag);
  }

  return errors::codegen_call<typed_value_vector, codegen_value>(flag_vals, [this, module, &builder] (vector<typed_value> &flag_arr) -> codegen_value {
      vector<Value*> flag_bitmasks;

      //ensure that the types are all shader flags
      for (unsigned int flag_idx = 0; flag_idx < flags.size(); ++flag_idx) {
	type_spec ty = flag_arr[flag_idx].get<1>();
	if (ty != state->types["shader_flag"]) {
	  if (!flags[flag_idx]->bound() && ty != state->types["module"]) {
	    Value *v = flag_arr[flag_idx].get<0>().extract_value();
	    ty->destroy(v, module, builder);

	    return errors::make_error<errors::error_message>("All flag expressions must evaluate to 'shader_flag' type.", line_no, column_no);
	  }
	}

	Value *mask = flag_arr[flag_idx].get<0>().extract_value();
	flag_bitmasks.push_back(mask);
      }
      
      //bitwise OR all the masks
      Value *full_mask = ConstantInt::get(getGlobalContext(), APInt(64, 0, false));
      for (auto mask = flag_bitmasks.begin(); mask != flag_bitmasks.end(); ++mask) {
	full_mask = builder.CreateOr(full_mask, *mask);
      }

      return full_mask;
    });
}
