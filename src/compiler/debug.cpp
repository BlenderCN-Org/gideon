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

#include "compiler/debug.hpp"
#include "compiler/symboltable.hpp"

using namespace raytrace;
using namespace std;
using namespace llvm;

const string debug_state::producer_name = "gideon-compiler 0.1-alpha";

debug_state::debug_state(llvm::Module *m,
			 const string &file_name, const string &file_path,
			 bool do_optimize, bool do_emit_debug) :
  dbg_builder(*m),
  file_info(dbg_builder.createFile(file_name, file_path))
{
  dbg_builder.createCompileUnit(dwarf::DW_LANG_C_plus_plus,
				file_name, file_path,
				producer_name,
				do_optimize, "", 0);				
}

void debug_state::pop() {
  state.pop_back();
}

void debug_state::push_function(const function_entry &func,
				unsigned int line_no, unsigned int column_no) {
  const MDNode *scope = (state.size() == 0) ? dbg_builder.getCU() : state.back();

  vector<Value*> func_param_ty;
  for (auto it = func.arguments.begin(); it != func.arguments.end(); ++it) {
    func_param_ty.push_back(dbg_builder.createBasicType("int", sizeof(int)*8, 0, dwarf::DW_ATE_signed));
  }
  DIArray param_ty_arr = dbg_builder.getOrCreateArray(func_param_ty);
  DICompositeType func_ty = dbg_builder.createSubroutineType(file_info, param_ty_arr);

  DISubprogram f = dbg_builder.createFunction(DIDescriptor(scope),
					      func.name, func.full_name,
					      file_info, line_no,
					      func_ty, true, true, line_no);
  state.push_back(f);
}

void debug_state::set_location(IRBuilder<> &builder,
			       unsigned int line_no, unsigned int column_no) {
  DICompileUnit cu(dbg_builder.getCU());
  MDNode *scope = (state.size() == 0) ? cu : state.back();
  
  builder.SetCurrentDebugLocation(DebugLoc::get(line_no, column_no, scope));
}
