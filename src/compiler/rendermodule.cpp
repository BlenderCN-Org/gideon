#include "compiler/rendermodule.hpp"
#include "compiler/parser.hpp"

#include "rtlparser.hpp"
#include "rtlscanner.hpp"

#include "llvm/PassManager.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/DataLayout.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Attributes.h"

#include <stdexcept>

using namespace std;
using namespace raytrace;
using namespace llvm;

int yyparse(yyscan_t scanner, ast::gideon_parser_data *gd_data);

render_module::render_module(const string &name, const string &source_code) :
  name(name), source(source_code)
{
  initialize_types(parser.types);
  parse_source();
  load_exports();
}

render_module::~render_module() { }

void render_module::parse_source() {
  yyscan_t scanner;
  
  if (yylex_init(&scanner)) { throw runtime_error("Could not initialize yylex"); }
  YY_BUFFER_STATE state = yy_scan_string(source.c_str(), scanner);
  
  ast::gideon_parser_data gd_data { &parser, &top, &dependencies };
  if (yyparse(scanner, &gd_data)) { throw runtime_error("Parser error"); }  

  yy_delete_buffer(state, scanner);
  yylex_destroy(scanner);
}

void render_module::load_exports() {
  for (auto ast_it = top.begin(); ast_it != top.end(); ast_it++) {
    (*ast_it)->get_export_info();
  }
}

Module *render_module::compile() {
  Module *module = new Module(name.c_str(), getGlobalContext());
  IRBuilder<> builder(getGlobalContext());

  auto report_errors = [] (compile_error &err) -> codegen_void {
    cout << "--- Error Report ---" << endl;
    cout << err.what() << endl;
    exit(-1);
    
    return nullptr;
  };
  raytrace::errors::error_container_operation<codegen_void, codegen_void> report(report_errors);

  codegen_void result = nullptr;

  for (auto ast_it = top.begin(); ast_it != top.end(); ast_it++) {
    codegen_value gen_val = (*ast_it)->codegen(module, builder);
    codegen_void val = raytrace::errors::codegen_ignore_value(gen_val);
    result = errors::merge_void_values(result, val);
  }

  boost::apply_visitor(report, result);

  optimize(module);
  return module;
}

void render_module::optimize(Module *module) {
  FunctionPassManager fpm(module);

  // Set up the optimizer pipeline.  Start with registering info about how the
  // target lays out data structures.
  fpm.add(new DataLayout(module));  
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

  Module::FunctionListType &funcs = module->getFunctionList();
  for (auto it = funcs.begin(); it != funcs.end(); it++) fpm.run(*it);
}