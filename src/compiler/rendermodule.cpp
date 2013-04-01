#include "compiler/rendermodule.hpp"
#include "compiler/parser.hpp"

#include "rtlparser.hpp"
#include "rtlscanner.hpp"

#include <stdexcept>

using namespace std;
using namespace raytrace;
using namespace llvm;

int yyparse(yyscan_t scanner, ast::gideon_parser_data *gd_data);

render_module::render_module(const string &name, const string &source_code) :
  name(name), source(source_code)
{
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

  for (auto ast_it = top.begin(); ast_it != top.end(); ast_it++) {
    (*ast_it)->codegen(module, builder);
  }

  return module;
}
