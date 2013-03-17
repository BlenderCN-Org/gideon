#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/JIT.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/PassManager.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/DataLayout.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Attributes.h"

#include "math/vector.hpp"

#include "compiler/ast/ast.hpp"
#include "compiler/types.hpp"
#include "compiler/symboltable.hpp"

#include "rtlparser.hpp"
#include "rtlscanner.hpp"

#include <iostream>

using namespace std;
using namespace raytrace;
using namespace llvm;

struct rtl_parse_data {
  var_symbol_table *variables;
  func_symbol_table *functions;
  control_state *control;
  vector<ast::global_declaration_ptr> *ast;
};

int yyparse(yyscan_t scanner, rtl_parse_data *rtl_data);

void compile_string(const std::string &source,
		    var_symbol_table &vars, func_symbol_table &funcs,
		    control_state &control,
		    vector<ast::global_declaration_ptr> &ast) {
  yyscan_t scanner;

  if (yylex_init(&scanner)) { throw runtime_error("Could not initialize yylex"); }
  YY_BUFFER_STATE state = yy_scan_string(source.c_str(), scanner);

  rtl_parse_data data = { &vars, &funcs, &control, &ast };
  if (yyparse(scanner, &data)) { throw runtime_error("Parser error"); }  

  yy_delete_buffer(state, scanner);
  yylex_destroy(scanner);
}



int main(int argc, char **argv) {
  InitializeNativeTarget();
  LLVMContext &ctx = getGlobalContext();
  Module *module = new Module("shader_test", ctx);
  IRBuilder<> builder(ctx);
  
  string error_str;
  ExecutionEngine *engine = EngineBuilder(module).setErrorStr(&error_str).create();
  
  FunctionPassManager fpm(module);

  // Set up the optimizer pipeline.  Start with registering info about how the
  // target lays out data structures.
  fpm.add(new DataLayout(*engine->getDataLayout()));
  
  fpm.add(createPromoteMemoryToRegisterPass());


  // Provide basic AliasAnalysis support for GVN.
  fpm.add(createBasicAliasAnalysisPass());
  // Do simple "peephole" optimizations and bit-twiddling optzns.
  fpm.add(createInstructionCombiningPass());
  // Reassociate expressions.
  fpm.add(createReassociatePass());
  // Eliminate Common SubExpressions.
  fpm.add(createGVNPass());
  // Simplify the control flow graph (deleting unreachable blocks, etc).
  fpm.add(createCFGSimplificationPass());

  fpm.doInitialization();
  
  var_symbol_table symbols;
  func_symbol_table functions;
  control_state control;
  vector<ast::global_declaration_ptr> ast;
  
  compile_string("extern float sin(float x) : sinf;\n"
		 "void incr(output float x) { x = x + 1.0; }\n"
		 "void incr_int(output int i) { i = i + 1; }\n"
		 "int test_ints(int a, int m) {\n"
		 "  int v = 0;\n"
		 "  if (a < m) {\n"
		 "  for (int i = a; i < m; incr_int(i)) {\n"
		 "    v = v + 1;\n"
		 "    break;\n"
		 "  }\n"
		 "  }\n"
		 "  return v;\n"
		 "}\n"
		 "vec4 add_vectors(vec4 in) { return in + vec4(1.0, 1.0, 1.0, 1.0); }\n"
		 "float foo(float a, output float b, output vec4 in_x, output vec4 out_x) {\n"
		 "  incr(a);\n"
		 "  b = b + a;\n"
		 "  out_x = add_vectors(in_x);\n"
		 "  return a;\n"
		 "}\n"
		 "float test_sin(float x) { return sin(x); }\n",
		 symbols, functions, control, ast);
  
  for (auto gd_it = ast.begin(); gd_it != ast.end(); gd_it++) (*gd_it)->codegen(module, builder);
  
  bool do_optimize = true;
  if (do_optimize) {
    Function *f = functions.get("foo").func;
    fpm.run(*f);
    
    f = functions.get("incr").func;
    fpm.run(*f);
    
    f = functions.get("add_vectors").func;
    fpm.run(*f);

    f = functions.get("test_ints").func;
    fpm.run(*f);
  }
    
  verifyModule(*module);
  //module->dump();
  
  void *fptr;
  fptr = engine->getPointerToFunction(functions.get("foo").func);
  /*float (*foo)(float, float, bool, float) = (float (*)(float, float, bool, float))(fptr);
  
  float rt;
  float x = 2.2f, z = -0.56f;
  cout << "foo(x = " << x << ", z = " << z << ", false) = " << foo(x, z, false, rt) << " | Expected: " << 3.2 + x + x + z + 2.0 << endl;
  cout << "Pointer Size: " << sizeof(ast::expression_ptr) << endl;*/
  
  float (*foo)(float, float*, float4*, float4 *) = (float (*)(float, float*, float4*, float4*))(fptr);

  float4 x{1.0f, 2.0f, 3.0f, 4.0f};
  float4 y;

  float a = 0.0;
  float b = 5.0;
  cout << "foo(a = " << a << ", &b, &x, &y) = " << foo(a, &b, &x, &y) << endl;
  cout << "b = " << b << endl;
  cout << "X: {" << x.x << ", " << x.y << ", " << x.z << ", " << x.w << "}" << endl;
  cout << "Y: {" << y.x << ", " << y.y << ", " << y.z << ", " << y.w << "}" << endl;

  fptr = engine->getPointerToFunction(functions.get("test_ints").func);
  int (*test_ints)(int, int) = (int (*)(int, int))(fptr);
  int i = 1;
  int m = 8;
  cout << "test_ints(i = " << i << ", m = " << m << ") = " << test_ints(i, m) << endl;

  fptr = engine->getPointerToFunction(functions.get("test_sin").func);
  float (*test_sin)(float) = (float (*)(float))(fptr);

  a = 1.2;
  cout << "test_sin(a = " << a << ") = " << test_sin(a) << endl;

  delete engine;
  return 0;
}
