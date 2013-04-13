#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/JIT.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/PassManager.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/DataLayout.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Attributes.h"

#include "math/vector.hpp"

#include "compiler/rendermodule.hpp"

#include <fstream>
#include <iostream>

using namespace std;
using namespace raytrace;
using namespace llvm;

int main(int argc, char **argv) {
  if (argc < 2) {
    cerr << "Usage: " << argv[0] << " <source-file-name>" << endl;
    return -1;
  }

  InitializeNativeTarget();
  
  ifstream render_file(argv[1]);
  string source((istreambuf_iterator<char>(render_file)), istreambuf_iterator<char>());
  cout << "Source Code: " << endl << source << endl;
  render_module render2("test_loop", source);
  Module *module2 = render2.compile();
  module2->dump();
  verifyModule(*module2);
  
  //delete engine;
  return 0;
}
