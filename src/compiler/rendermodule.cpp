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
#include "llvm/Transforms/IPO.h"

#include "llvm/Attributes.h"
#include "llvm/Linker.h"

#include <fstream>
#include <stdexcept>

#include <boost/filesystem.hpp>

using namespace std;
using namespace raytrace;
using namespace llvm;

/* Source File Loading */

string raytrace::basic_path_resolver(const string &src_path_str,
				     const vector<string> &search_paths) {
  boost::filesystem::path src_path(src_path_str);
  if (!src_path.has_extension()) src_path.replace_extension(".gdl");
  
  vector<boost::filesystem::path> paths_to_check;
  if (src_path.is_absolute()) paths_to_check.push_back(src_path);
  else {
    //check current directory and search paths
    for (auto search_it : search_paths) {
      paths_to_check.push_back(search_it / src_path);
    }

    paths_to_check.push_back(boost::filesystem::current_path() / src_path);
  }

  for (auto it = paths_to_check.begin(); it != paths_to_check.end(); ++it) {
    if (boost::filesystem::exists(*it) &&
	boost::filesystem::is_regular_file(*it)) {
      return it->native();
    }
  }

  stringstream err_ss;
  err_ss << "Unable to resolve path: " << src_path << endl;
  throw runtime_error(err_ss.str());
}

string raytrace::basic_source_loader(const string &abs_path) {
  ifstream file(abs_path);
  if (!file.is_open()) {
    stringstream err_ss;
    err_ss << "Unable to load source file: " << abs_path << endl;
    throw runtime_error(err_ss.str());
  }
  
  return string((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
}

/* Render Object */

render_object::render_object(const string &name, const string &source_code) :
  name(name), source_code(source_code)
{
  
}

int yyparse(yyscan_t scanner, ast::gideon_parser_data *gd_data);

codegen_void render_object::parse(ast::parser_state *parser,
				  /* out */ vector<ast::global_declaration_ptr> &syntax_tree,
				  /* out */ vector<string> &object_dependencies) {
  yyscan_t scanner;
  
  if (yylex_init(&scanner)) { throw runtime_error("Could not initialize yylex"); }
  YY_BUFFER_STATE state = yy_scan_string(source_code.c_str(), scanner);
  yyset_lineno(1, scanner);
  
  ast::gideon_parser_data gd_data { parser, &syntax_tree, &object_dependencies };
  codegen_void result = empty_type();

  try {
    if (yyparse(scanner, &gd_data)) { throw runtime_error("Parser error"); }  
  }
  catch (compile_error &e) {
    result = e;
  }

  yy_delete_buffer(state, scanner);
  yylex_destroy(scanner);

  //add a global scene pointer declaration
  ast::type_expr_ptr scene_type = ast::type_expr_ptr(new ast::typename_expression(parser, "scene_ptr", 0, 0));
  syntax_tree.insert(syntax_tree.begin(), ast::global_declaration_ptr(new ast::global_variable_decl(parser, "__gd_scene", scene_type, nullptr, 0, 0)));
  return result;
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

/* Compiled Renderer */

compiled_renderer::compiled_renderer(Module *module) :
  module(module),
  engine(EngineBuilder(module).create())
{
  
}

void *compiled_renderer::get_function_pointer(const string &func_name) {
  return engine->getPointerToFunction(module->getFunction(func_name));
}

void compiled_renderer::map_global(const string &name, void *location_ptr) {
  //engine->addGlobalMapping(cast<GlobalVariable>(module->getNamedGlobal(name)), location_ptr);
  engine->updateGlobalMapping(cast<GlobalVariable>(module->getNamedGlobal(name)), location_ptr);
}

/* Render Program */

render_program::render_program(const string &name) :
  name(name),
  path_resolver([this] (const string &path) -> string { return path; }),
  source_loader([this] (const string &path) -> string { return default_loader(path); })
{
  initialize_types(types);
}

render_program::render_program(const string &name,
			       const boost::function<string (const string &)> &resolver,
			       const boost::function<string (const string &)> &loader) :
  name(name),
  path_resolver(resolver),
  source_loader(loader)
{
  initialize_types(types);
}

render_program::object_entry::object_entry(render_program *prog, const render_object &obj) :
  obj(obj),
  parser(prog->types)
{
  type_conversion_table::initialize_standard_conversions(parser.type_conversions, parser.types);
  binop_table::initialize_standard_ops(parser.binary_operations,
				       parser.types);
  unary_op_table::initialize_standard_ops(parser.unary_operations,
					  parser.types);
  parser.objects = prog;
}

void render_program::add_object(const render_object &obj) {
  objects[obj.name] = std::shared_ptr<object_entry>(new object_entry(this, obj));
}

string render_program::default_loader(const string &path) {
  ifstream file(path);
  if (!file.is_open()) {
    stringstream err_ss;
    err_ss << "Unable to load source file: " << path << endl;
    throw runtime_error(err_ss.str());
  }
  
  string source((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
  return source;
}

void render_program::load_source_file(const string &obj_name) {
  string full_name = get_object_name(obj_name);
  load_resolved_filename(full_name);
}

string render_program::get_object_name(const string &path) {
  return path_resolver(path);
}

void render_program::load_resolved_filename(const string &path) {
  string source = source_loader(path);
  add_object(render_object(path, source));
}

codegen_void render_program::object_entry::parse() {
  return obj.parse(&parser, syntax_tree, dependencies);
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
      string full_dep_name = get_object_name(*dep_it);
      need_to_load.push_back(full_dep_name);
    }
  }

  //now make sure all dependencies are satisfies
  while (need_to_load.size() > 0) {
    string dep_name = need_to_load.back();
    need_to_load.pop_back();
    
    if (!has_object(dep_name)) {
      //add dependencies to the list and load this source file
      load_resolved_filename(dep_name);
      std::shared_ptr<object_entry> &object = objects[dep_name];
      codegen_void parse_result = object->parse();
      errors::extract_left(parse_result);
      
      for (auto dep_it = object->dependencies.begin(); dep_it != object->dependencies.end(); ++dep_it) {
	string full_dep_name = get_object_name(*dep_it);
	need_to_load.push_back(full_dep_name);
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

  dep_graph_type dep_graph(dep_list.begin(), dep_list.end(), id_object_map.size());

  //generate an ID list in topological order (boost docs say it's in reverse, but that doesn't appear to be the case...)
  vector<int> id_order;
  boost::topological_sort(dep_graph, std::back_inserter(id_order));

  //compile the final list of names
  vector<string> order;
  for (auto it = id_order.rbegin(); it != id_order.rend(); ++it) {
    order.push_back(id_object_map[*it]);
  }

  return order;
}

Module *render_program::compile() {
  //parse all objects
  for (auto obj_it = objects.begin(); obj_it != objects.end(); ++obj_it) {
    std::shared_ptr<object_entry> &object = obj_it->second;
    codegen_void parse_result = object->parse();
    errors::extract_left(parse_result);
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
    
    string error_msg;
    bool link_st = linker.LinkInModule(module, &error_msg);
    if (link_st) {
      stringstream link_err;
      link_err << "Linker Error: " << error_msg;
      throw runtime_error(link_err.str());
    }
  }

  Module *result = linker.releaseModule();
  optimize(result);
  return result;
}

void render_program::foreach_function_type(exports::function_export::export_type type,
					   const boost::function<void (const std::string &, const std::string &)> &on_function) const {
  boost::unordered_set<string> names_seen;
  for (auto obj_it = objects.begin(); obj_it != objects.end(); ++obj_it) {
    obj_it->second->parser.exports.foreach_function_type(type, names_seen, on_function);
  }
}

void render_program::optimize(Module *module) {
  PassManager pm;
  
  // Set up the optimizer pipeline.  Start with registering info about how the
  // target lays out data structures.
  pm.add(new DataLayout(module));  
  
  pm.add(createFunctionInliningPass());
  pm.add(createPromoteMemoryToRegisterPass());
  
  // Provide basic AliasAnalysis support for GVN.
  pm.add(createBasicAliasAnalysisPass());
  // Do simple "peephole" optimizations and bit-twiddling optzns.
  pm.add(createInstructionCombiningPass());
  // Reassociate expressions.
  pm.add(createReassociatePass());
  // Eliminate Common SubExpressions.
  pm.add(createGVNPass());
  // Simplify the control flow graph (deleting unreachable blocks, etc).
  pm.add(createCFGSimplificationPass());
  
  pm.run(*module);
}
