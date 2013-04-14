#include "compiler/ast/distribution.hpp"

using namespace raytrace;
using namespace std;
using namespace llvm;

ast::distribution::distribution(parser_state *st, const string &name,
				const vector<function_argument> &params,
				const vector<global_declaration_ptr> &internal_decl,
				unsigned int line_no, unsigned int column_no) :
  global_declaration(st),
  name(name), params(params),
  internal_decl(internal_decl)
{
  
}

StructType *ast::distribution::getParameterType() {
  vector<Type*> param_types;
  boost::unordered_map<string, int> name_idx_map;

  for (unsigned int i = 0; i < params.size(); i++) {
    function_argument &p = params[i];
    param_types.push_back(p.type->llvm_type());
    name_idx_map[p.name] = static_cast<int>(i);
  }

  string pstruct_name = name + string("_params");
  return StructType::create(getGlobalContext(), param_types, pstruct_name);
}

codegen_value ast::distribution::codegen(Module *module, IRBuilder<> &builder) {
  //enter new scope

  //define a struct type containing all parameters
  //create variable names pointing to each member of the param struct

  //evaluate all internal declarations

  //find the 'evaluate' function (error if not declared)
  //TODO: How should we pass the param struct to these functions??

  //define an externally visible function to instantiate a param struct

  //define an externally visible function to evaluate this distribution

  //leave new scope
}
