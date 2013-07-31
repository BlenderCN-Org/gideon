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

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/JIT.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/PassManager.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/IR/Attributes.h"

#include "math/vector.hpp"

#include "compiler/rendermodule.hpp"

#include <fstream>
#include <iostream>

#include <boost/filesystem.hpp>

using namespace std;
using namespace raytrace;
using namespace llvm;

static const bool gideon_debug = false;

boost::filesystem::path std_search_path = boost::filesystem::path(__FILE__).parent_path().parent_path() / "src" / "standard";

int main(int argc, char **argv) {
  if (argc < 2) {
    cerr << "Usage: " << argv[0] << " <source-file-name> [entry-point]" << endl;
    return -1;
  }
  
  boost::filesystem::path file_dir = boost::filesystem::path(argv[1]).parent_path();
  
  vector<string> search_paths;
  search_paths.push_back(std_search_path.native());
  search_paths.push_back(file_dir.native());

  InitializeNativeTarget();
  InitializeNativeTargetAsmPrinter();
  InitializeNativeTargetAsmParser();
  
  render_program prog("test_program",
		      true, false,
		      [search_paths] (const string &fname) -> string { return basic_path_resolver(fname, search_paths); },
		      basic_source_loader);
  prog.load_source_file(argv[1]);
  
  Module *module = prog.compile();
  verifyModule(*module);
  module->dump();
  
  if (argc >= 3) {
    if (gideon_debug) {
      Function *func = module->getFunction(argv[2]);
      func->viewCFG();
    }
    else {
      compiled_renderer kernel(module);
      void *entry_ptr = kernel.get_function_pointer(argv[2]);
      void (*entry_func)() = (void (*)())(entry_ptr);
      entry_func();
    }
  }
  
  return 0;
}
