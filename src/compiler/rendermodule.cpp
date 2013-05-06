#include "compiler/rendermodule.hpp"
#include "compiler/parser.hpp"

#include "rtlparser.hpp"
#include "rtlscanner.hpp"

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/topological_sort.hpp>

#include "llvm/PassManager.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/DataLayout.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Attributes.h"
#include "llvm/Linker.h"

#include <fstream>
#include <stdexcept>

using namespace std;
using namespace raytrace;
using namespace llvm;

/* Render Object */

render_object::render_object(const string &name, const string &source_code) :
  name(name), source_code(source_code)
{
  
}

int yyparse(yyscan_t scanner, ast::gideon_parser_data *gd_data);

void render_object::parse(ast::parser_state *parser,
			  /* out */ vector<ast::global_declaration_ptr> &syntax_tree,
			  /* out */ vector<string> &object_dependencies) {
  yyscan_t scanner;
  
  if (yylex_init(&scanner)) { throw runtime_error("Could not initialize yylex"); }
  YY_BUFFER_STATE state = yy_scan_string(source_code.c_str(), scanner);
  yyset_lineno(1, scanner);
  
  ast::gideon_parser_data gd_data { parser, &syntax_tree, &object_dependencies };
  if (yyparse(scanner, &gd_data)) { throw runtime_error("Parser error"); }  

  yy_delete_buffer(state, scanner);
  yylex_destroy(scanner);

  //add a global scene pointer declaration
  syntax_tree.insert(syntax_tree.begin(), ast::global_declaration_ptr(new ast::global_variable_decl(parser, "__gd_scene", parser->types["scene_ptr"])));
}

Module *render_object::compile(const string &name, ast::parser_state *parser,
			       vector<ast::global_declaration_ptr> &syntax_tree) {
  Module *module = new Module(name.c_str(), getGlobalContext());
  IRBuilder<> builder(getGlobalContext());
  
  parser->modules.scope_push(); //start with an anonymous top-level module

  codegen_vector result;
  for (auto node_it = syntax_tree.begin(); node_it != syntax_tree.end(); ++node_it) {
    codegen_value v = (*node_it)->codegen(module, builder);
    result = errors::codegen_vector_push_back(result, v);
  }

  parser->modules.scope_pop(module, builder, false);

  errors::extract_left(result);
  return module;
}

/* Render Program */

render_program::render_program(const string &name) :
  name(name)
{
  initialize_types(types);
}

render_program::object_entry::object_entry(render_program *prog, const render_object &obj) :
  obj(obj),
  parser(prog->types)
{
  //initialize_types(parser.types);
  binop_table::initialize_standard_ops(parser.binary_operations,
				       parser.types);
  parser.objects = prog;
}

void render_program::add_object(const render_object &obj) {
  objects[obj.name] = std::shared_ptr<object_entry>(new object_entry(this, obj));
}

void render_program::load_source_file(const string &path) {
  ifstream file(path);
  if (!file.is_open()) {
    stringstream err_ss;
    err_ss << "Unable to load source file: " << path << endl;
    throw runtime_error(err_ss.str());
  }

  string source((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
  add_object(render_object(path, source));
}

void render_program::object_entry::parse() {
  obj.parse(&parser, syntax_tree, dependencies);
}

bool render_program::has_object(const string &name) {
  return (objects.find(name) != objects.end());
}

export_table &render_program::get_export_table(const string &name) {
  auto it = objects.find(name);
  assert(it != objects.end());

  return it->second->parser.exports;
}

void render_program::load_all_dependencies() {
  vector<string> need_to_load;

  //populate the list with each object's dependencies
  for (auto obj_it = objects.begin(); obj_it != objects.end(); ++obj_it) {
    std::shared_ptr<object_entry> &object = obj_it->second;

    for (auto dep_it = object->dependencies.begin(); dep_it != object->dependencies.end(); ++dep_it) {
      need_to_load.push_back(*dep_it);
    }
  }

  //now make sure all dependencies are satisfies
  while (need_to_load.size() > 0) {
    string dep_name = need_to_load.back();
    need_to_load.pop_back();

    if (!has_object(dep_name)) {
      //add dependencies to the list and load this source file
      load_source_file(dep_name);
      std::shared_ptr<object_entry> &object = objects[dep_name];
      object->parse();
      
      for (auto dep_it = object->dependencies.begin(); dep_it != object->dependencies.end(); ++dep_it) {
	need_to_load.push_back(*dep_it);
      }
    }
  }
}

vector<string> render_program::generate_compile_order() {
  typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS> dep_graph_type;
  
  //generate a mapping from integer ID's to object names
  map<string, int> object_id_map;
  vector<string> id_object_map;
  
  for (auto obj_it = objects.begin(); obj_it != objects.end(); ++obj_it) {
    int id = static_cast<int>(id_object_map.size());
    object_id_map[obj_it->first] = id;
    id_object_map.push_back(obj_it->first);
  }

  //build dependency graph
  vector<pair<int, int>> dep_list;
  for (auto obj_it = objects.begin(); obj_it != objects.end(); ++obj_it) {
    int dst_id = object_id_map[obj_it->first];
    std::shared_ptr<object_entry> &object = obj_it->second;

    for (auto dep_it = object->dependencies.begin(); dep_it != object->dependencies.end(); ++dep_it) {
      int src_id = object_id_map[*dep_it];
      dep_list.push_back(make_pair(src_id, dst_id));
    }
  }
  dep_list.clear();
  dep_graph_type dep_graph(dep_list.begin(), dep_list.end(), id_object_map.size());

  //generate an ID list in topological order (boost docs say it's in reverse, but that doesn't appear to be the case...)
  vector<int> id_order;
  boost::topological_sort(dep_graph, std::back_inserter(id_order));

  //compile the final list of names
  vector<string> order;
  for (auto it = id_order.begin(); it != id_order.end(); ++it) {
    order.push_back(id_object_map[*it]);
  }

  return order;
}

Module *render_program::compile() {
  //parse all objects
  for (auto obj_it = objects.begin(); obj_it != objects.end(); ++obj_it) {
    std::shared_ptr<object_entry> &object = obj_it->second;
    object->parse();
  }
  
  //load and parse all dependency objects
  load_all_dependencies();

  //using the dependency graph, sort all objects in topological order
  //this way, during code generation we know that each object's
  //dependencies will have been satisfied when we perform code generation
  vector<string> order = generate_compile_order();
  
  //perform code generation and linking
  Linker linker(name, name, getGlobalContext());

  for (auto name_it = order.begin(); name_it != order.end(); ++name_it) {
    cout << "Compiling Object: " << *name_it << endl;
    std::shared_ptr<object_entry> &object = objects[*name_it];
    Module *module = render_object::compile(object->obj.name, &object->parser, object->syntax_tree);
    linker.LinkInModule(module);
  }

  Module *result = linker.releaseModule();
  optimize(result);

  return result;
}

void render_program::optimize(Module *module) {
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

/* Render Module */

render_module::render_module(const string &name, const string &source_code) :
  name(name), source(source_code), parser(types)
{
  initialize_types(types);
  binop_table::initialize_standard_ops(parser.binary_operations,
				       parser.types);
  parse_source();
}

render_module::~render_module() { }

void render_module::parse_source() {
  yyscan_t scanner;
  
  if (yylex_init(&scanner)) { throw runtime_error("Could not initialize yylex"); }
  YY_BUFFER_STATE state = yy_scan_string(source.c_str(), scanner);
  yyset_lineno(1, scanner);
  
  ast::gideon_parser_data gd_data { &parser, &top, &dependencies };
  if (yyparse(scanner, &gd_data)) { throw runtime_error("Parser error"); }  

  yy_delete_buffer(state, scanner);
  yylex_destroy(scanner);

  //add a global scene pointer declaration
  top.insert(top.begin(), ast::global_declaration_ptr(new ast::global_variable_decl(&parser, "__gd_scene", parser.types["scene_ptr"])));
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
  
  parser.modules.scope_push();
  
  for (auto ast_it = top.begin(); ast_it != top.end(); ast_it++) {
    codegen_value gen_val = (*ast_it)->codegen(module, builder);
    codegen_void val = raytrace::errors::codegen_ignore_value(gen_val);
    result = errors::merge_void_values(result, val);
  }

  parser.modules.scope_pop(module, builder, false);
  
  boost::apply_visitor(report, result);

  parser.exports.dump();

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

//vector<distribution_export> render_module::get_distributions() const {
//  return vector<distribution_export>();
//}
