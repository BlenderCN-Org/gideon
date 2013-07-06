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

boost::filesystem::path std_search_path = boost::filesystem::path(__FILE__).parent_path().parent_path() / "src" / "standard";

int main(int argc, char **argv) {
  if (argc < 2) {
    cerr << "Usage: " << argv[0] << " <source-file-name>" << endl;
    return -1;
  }
  
  vector<string> search_paths;
  search_paths.push_back(std_search_path.native());

  InitializeNativeTarget();
  
  render_program prog("test_program",
		      [search_paths] (const string &fname) -> string { return basic_path_resolver(fname, search_paths); },
		      basic_source_loader);
  prog.load_source_file(argv[1]);
  
  Module *module = prog.compile();
  verifyModule(*module);
  module->dump();
  
  //delete engine;
  return 0;
}
