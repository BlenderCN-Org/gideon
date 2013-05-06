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
  
  render_program prog("test_program");
  prog.load_source_file(argv[1]);
  
  Module *module = prog.compile();
  verifyModule(*module);
  module->dump();
  
  //delete engine;
  return 0;
}
