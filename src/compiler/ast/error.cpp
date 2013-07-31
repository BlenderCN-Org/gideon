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

#include "compiler/ast/error.hpp"
#include "compiler/debug.hpp"

#include <typeinfo>

using namespace std;
using namespace raytrace;
using namespace llvm;

ast::error::error(parser_state *st,
		  const expression_ptr &error_str,
		  unsigned int line_no, unsigned int column_no):
  statement(st, line_no, column_no),
  error_str(error_str)
{
  
}

codegen_void ast::error::codegen(Module *module, IRBuilder<> &builder) {
  type_spec str_ty = state->types["string"];

  typed_value_container msg = error_str->codegen(module, builder);
  state->dbg->set_location(builder, line_no, column_no);

  code_value msg_str = typecast(msg, str_ty,
				false, !error_str->bound(), module, builder);
  vector<Type*> throw_arg_ty{str_ty->llvm_ptr_type()};
  FunctionType *throw_ty = FunctionType::get(Type::getVoidTy(getGlobalContext()), throw_arg_ty, false);
  Function *throw_func = cast<Function>(module->getOrInsertFunction("gd_builtin_error", throw_ty));
  
  return errors::codegen_call<code_value, codegen_void>(msg_str, [&] (value &msg_val) -> codegen_void {
      Value *error_ptr = str_ty->allocate(module, builder);
      Value *msg_copy = str_ty->copy(msg_val.extract_value(), module, builder);
      str_ty->store(msg_copy, error_ptr, module, builder);
      
      BasicBlock *lp_block = generate_landing_pad(module);

      control_state::function_state &func_st = state->control.get_function_state();
      BasicBlock *next_block = BasicBlock::Create(getGlobalContext(), "continue", func_st.func);
      
      builder.CreateInvoke(throw_func, next_block, lp_block, error_ptr);
      builder.SetInsertPoint(next_block);
      
      return empty_type();
    });
}
