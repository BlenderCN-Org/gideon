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

//Floats

Type *float_type::llvm_type() const {
  return Type::getFloatTy(getGlobalContext());
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

typed_value_container string_type::initialize(Module *module, IRBuilder<> &builder) const {
  return typed_value(builder.CreateInsertValue(UndefValue::get(llvm_type()),
					       ConstantInt::get(getGlobalContext(), APInt(1, true, true)), ArrayRef<unsigned int>(0), "init_str"),
		     types->at("string"));
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
  
  return empty_type();
}

//Scene Pointer

Type *scene_ptr_type::llvm_type() const {
  return Type::getIntNPtrTy(getGlobalContext(), 8*sizeof(void*));
}
