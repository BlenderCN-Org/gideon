#include "compiler/ast/typename.hpp"

using namespace std;
using namespace raytrace;

typecheck_value ast::typename_expression::codegen_type() {
  if (path.size() == 0) return typename_lookup(name);

  auto path_it = path.begin();
  typed_value_container top_module = variable_lookup(*path_it);
  
  return errors::codegen_call<typed_value_container, typecheck_value>(top_module, [&] (typed_value &m) -> typecheck_value {
      type *t = m.get<1>();
      if (t != state->types["module"]) {
	stringstream err_ss;
	err_ss << "No such module named '" << *path_it << "'";
	return errors::make_error<errors::error_message>(err_ss.str(), line_no, column_no);
      }
      
      module_ptr mod = m.get<0>().extract_module();
      ++path_it;
      
      while (path_it != path.end()) {
	//lookup the next module in the path
	auto mod_it = mod->modules.find(*path_it);
	if (mod_it == mod->modules.end()) {
	  stringstream err_ss;
	  err_ss << "No such module named '" << *path_it << "'";
	  return errors::make_error<errors::error_message>(err_ss.str(), line_no, column_no);
	}

	mod = mod_it->second;
	++path_it;
      }

      //lookup the actual typee
      auto type_it = mod->types.find(name);
      if (type_it == mod->types.end()) {
	stringstream err_ss;
	err_ss << "Module has no type named '" << name << "'";
	return errors::make_error<errors::error_message>(err_ss.str(), line_no, column_no);
      }
      
      return mod->types.extract_entry(type_it);
    });
}

typecheck_value ast::array_type_expression::codegen_type() {
  typecheck_value base_ty = base->codegen_type();
  return errors::codegen_call(base_ty, [this] (type_spec &t) -> typecheck_value {
      return state->types.get_array(t, N);
    });
}

typecheck_value ast::array_ref_expression::codegen_type() {
  typecheck_value base_ty = base->codegen_type();
  return errors::codegen_call(base_ty, [this] (type_spec &t) -> typecheck_value {
      return state->types.get_array_ref(t);
    });
}


