#include "compiler/types/primitive.hpp"
#include "compiler/operations.hpp"
#include "compiler/llvm_helper.hpp"

using namespace raytrace;
using namespace llvm;
using namespace std;

Type *void_type::llvm_type() const {
  return Type::getVoidTy(getGlobalContext());
}

Type *bool_type::llvm_type() const {
  return Type::getInt1Ty(getGlobalContext());
}

//Ints

Type *int_type::llvm_type() const {
  return Type::getInt32Ty(getGlobalContext());
}

codegen_value int_type::op_add(Module *module, IRBuilder<> &builder,
			       codegen_value &lhs, codegen_value &rhs) const {
  typedef errors::argument_value_join<codegen_value, codegen_value>::result_value_type arg_val_type;
  boost::function<codegen_value (arg_val_type &)> op = [&builder, module] (arg_val_type &val) {
    return builder.CreateAdd(val.get<0>(), val.get<1>(), "i_add_tmp");
  };
  return errors::codegen_call_args(op, lhs, rhs);
}

codegen_value int_type::op_sub(Module *module, IRBuilder<> &builder,
			       codegen_value &lhs, codegen_value &rhs) const {
  typedef errors::argument_value_join<codegen_value, codegen_value>::result_value_type arg_val_type;
  boost::function<codegen_value (arg_val_type &)> op = [&builder, module] (arg_val_type &val) {
    return builder.CreateSub(val.get<0>(), val.get<1>(), "i_sub_tmp");
  };
  return errors::codegen_call_args(op, lhs, rhs);
}

codegen_value int_type::op_mul(Module *module, IRBuilder<> &builder,
				 codegen_value &lhs, codegen_value &rhs) const {
  typedef errors::argument_value_join<codegen_value, codegen_value>::result_value_type arg_val_type;
  boost::function<codegen_value (arg_val_type &)> op = [&builder, module] (arg_val_type &val) {
    return builder.CreateMul(val.get<0>(), val.get<1>(), "i_mul_tmp");
  };
  return errors::codegen_call_args(op, lhs, rhs);
}

codegen_value int_type::op_div(Module *module, IRBuilder<> &builder,
			       codegen_value &lhs, codegen_value &rhs) const {
  typedef errors::argument_value_join<codegen_value, codegen_value>::result_value_type arg_val_type;
  boost::function<codegen_value (arg_val_type &)> op = [&builder, module] (arg_val_type &val) {
    return builder.CreateSDiv(val.get<0>(), val.get<1>(), "i_div_tmp");
  };
  return errors::codegen_call_args(op, lhs, rhs);
}

codegen_value int_type::op_less(Module *module, IRBuilder<> &builder,
				  codegen_value &lhs, codegen_value &rhs) const {
  typedef errors::argument_value_join<codegen_value, codegen_value>::result_value_type arg_val_type;
  boost::function<codegen_value (arg_val_type &)> op = [&builder, module] (arg_val_type &val) {
    return builder.CreateICmpSLT(val.get<0>(), val.get<1>(), "i_lt_tmp");
  };
  return errors::codegen_call_args(op, lhs, rhs);
}

//Floats

Type *float_type::llvm_type() const {
  return Type::getFloatTy(getGlobalContext());
}

codegen_value float_type::op_add(Module *module, IRBuilder<> &builder,
				 codegen_value &lhs, codegen_value &rhs) const {
  typedef errors::argument_value_join<codegen_value, codegen_value>::result_value_type arg_val_type;
  boost::function<codegen_value (arg_val_type &)> op = [&builder, module] (arg_val_type &val) {
    return builder.CreateFAdd(val.get<0>(), val.get<1>(), "f_add_tmp");
  };
  return errors::codegen_call_args(op, lhs, rhs);
}

codegen_value float_type::op_sub(Module *module, IRBuilder<> &builder,
				 codegen_value &lhs, codegen_value &rhs) const {
  typedef errors::argument_value_join<codegen_value, codegen_value>::result_value_type arg_val_type;
  boost::function<codegen_value (arg_val_type &)> op = [&builder, module] (arg_val_type &val) {
    return builder.CreateFSub(val.get<0>(), val.get<1>(), "f_sub_tmp");
  };
  return errors::codegen_call_args(op, lhs, rhs);
}

codegen_value float_type::op_mul(Module *module, IRBuilder<> &builder,
				 codegen_value &lhs, codegen_value &rhs) const {
  typedef errors::argument_value_join<codegen_value, codegen_value>::result_value_type arg_val_type;
  boost::function<codegen_value (arg_val_type &)> op = [&builder, module] (arg_val_type &val) {
    return builder.CreateFMul(val.get<0>(), val.get<1>(), "f_mul_tmp");
  };
  return errors::codegen_call_args(op, lhs, rhs);
}

codegen_value float_type::op_div(Module *module, IRBuilder<> &builder,
				 codegen_value &lhs, codegen_value &rhs) const {
  typedef errors::argument_value_join<codegen_value, codegen_value>::result_value_type arg_val_type;
  boost::function<codegen_value (arg_val_type &)> op = [&builder, module] (arg_val_type &val) {
    return builder.CreateFDiv(val.get<0>(), val.get<1>(), "f_div_tmp");
  };
  return errors::codegen_call_args(op, lhs, rhs);
}

codegen_value float_type::op_less(Module *module, IRBuilder<> &builder,
				  codegen_value &lhs, codegen_value &rhs) const {
  typedef errors::argument_value_join<codegen_value, codegen_value>::result_value_type arg_val_type;
  boost::function<codegen_value (arg_val_type &)> op = [&builder, module] (arg_val_type &val) {
    return builder.CreateFCmpOLT(val.get<0>(), val.get<1>(), "f_lt_tmp");
  };
  return errors::codegen_call_args(op, lhs, rhs);
}

//String

string_type::string_type(type_table *types) :
  type(types, "string", "s"),
  str_type_value(StructType::create(getGlobalContext(),
				    ArrayRef<Type*>(vector<Type*>{
					Type::getInt1Ty(getGlobalContext()),
					  Type::getInt8PtrTy(getGlobalContext())}),
				    "string", false))
{

}

Type *string_type::llvm_type() const { return str_type_value; }

codegen_value string_type::initialize(Module *module, IRBuilder<> &builder) const {
  return builder.CreateInsertValue(UndefValue::get(llvm_type()),
				   ConstantInt::get(getGlobalContext(), APInt(1, true, true)), ArrayRef<unsigned int>(0), "init_str");
}

Value *string_type::copy(Value *value, Module *module, IRBuilder<> &builder) {
  Value *src_is_const = builder.CreateExtractValue(value, ArrayRef<unsigned int>(0), "str_is_const");
  Value *src_ptr = builder.CreateExtractValue(value, ArrayRef<unsigned int>(1), "str_data");
  
  Value *dst = CreateEntryBlockAlloca(builder, llvm_type(), "dst_str");
  Value *dst_is_const = builder.CreateStructGEP(dst, 0, "str_is_const");
  Value *dst_ptr = builder.CreateStructGEP(dst, 1, "str_data");

  Type *bool_type = Type::getInt1Ty(getGlobalContext());
  Type *ptr_type = Type::getInt8PtrTy(getGlobalContext());
  vector<Type*> arg_ty{bool_type, ptr_type, bool_type->getPointerTo(), ptr_type->getPointerTo()};

  FunctionType *ft = FunctionType::get(Type::getVoidTy(getGlobalContext()),
				       ArrayRef<Type*>(arg_ty), false);
  Function *copy = cast<Function>(module->getOrInsertFunction("gd_builtin_copy_string", ft));

  builder.CreateCall(copy, vector<Value*>{src_is_const, src_ptr, dst_is_const, dst_ptr});
  return builder.CreateLoad(dst, "str_copy");
}

codegen_void string_type::destroy(Value *value, Module *module, IRBuilder<> &builder) {
  Value *str = builder.CreateLoad(value, "str_ref");
  vector<Type*> arg_ty{Type::getInt1Ty(getGlobalContext()), Type::getInt8PtrTy(getGlobalContext())};
  FunctionType *ft = FunctionType::get(Type::getVoidTy(getGlobalContext()),
				       ArrayRef<Type*>(arg_ty), false);
  Function *dtor = cast<Function>(module->getOrInsertFunction("gd_builtin_destroy_string", ft));
  Value *is_const = builder.CreateExtractValue(str, ArrayRef<unsigned int>(0), "str_is_const");
  Value *str_ptr = builder.CreateExtractValue(str, ArrayRef<unsigned int>(1), "str_data");
  builder.CreateCall(dtor, vector<Value*>{is_const, str_ptr});
  
  return nullptr;
}

codegen_value string_type::op_add(Module *module, IRBuilder<> &builder,
				  codegen_value &lhs, codegen_value &rhs) const {
  typedef errors::argument_value_join<codegen_value, codegen_value>::result_value_type arg_val_type;
  boost::function<codegen_value (arg_val_type &)> add_op = [this, module, &builder] (arg_val_type &arg) -> codegen_value {
    Value *lhs = arg.get<0>();
    Value *rhs = arg.get<1>();

    Value *l_data = builder.CreateExtractValue(lhs, ArrayRef<unsigned int>(1), "s0_data");
    Value *r_data = builder.CreateExtractValue(rhs, ArrayRef<unsigned int>(1), "s1_data");

    Type *char_ptr = Type::getInt8PtrTy(getGlobalContext());
    vector<Type*> arg_ty{char_ptr, char_ptr};
    FunctionType *ft = FunctionType::get(char_ptr, ArrayRef<Type*>(arg_ty), false);
    Function *adder = cast<Function>(module->getOrInsertFunction("gd_builtin_concat_string", ft));

    Value *new_data = builder.CreateCall(adder, vector<Value*>{l_data, r_data});
    Value *new_str = builder.CreateInsertValue(UndefValue::get(llvm_type()),
					       ConstantInt::get(getGlobalContext(), APInt(1, false, true)), ArrayRef<unsigned int>(0), "str_concat_tmp");
    return builder.CreateInsertValue(new_str, new_data, ArrayRef<unsigned int>(1));    
  };

  return errors::codegen_call_args(add_op, lhs, rhs);
}

//Scene Pointer

Type *scene_ptr_type::llvm_type() const {
  return Type::getIntNPtrTy(getGlobalContext(), 8*sizeof(void*));
}
