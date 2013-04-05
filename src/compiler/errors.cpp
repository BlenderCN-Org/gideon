#include "compiler/errors.hpp"
#include <sstream>

using namespace raytrace;
using namespace llvm;
using namespace std;

Value *errors::codegen_value_extract(codegen_value &val) {
  return boost::apply_visitor(errors::return_or_throw(), val);
}

codegen_void errors::codegen_ignore_value(codegen_value &val) {
  auto op = [] (Value *&) -> codegen_void {
    return nullptr;
  };
  raytrace::errors::value_container_operation<codegen_value, codegen_void> ignore(op);
  return boost::apply_visitor(ignore, val);
}

namespace raytrace {
  namespace errors {
    template<>
    compile_error merge_errors(const compile_error &e0, const compile_error &e1) {
      stringstream ss;
      ss << e0.what() << endl << e1.what();
      return runtime_error(ss.str());
    }
  };
};

codegen_void errors::merge_void::operator()(compile_error &e0, compile_error &e1) const { return merge_errors<compile_error>(e0, e1); }

/*codegen_vector errors::codegen_vector_append::operator()(vector<Value*> &list, Value *&val) const {
  vector<Value*> result = list;
  result.push_back(val);
  return result;
}

codegen_vector errors::codegen_vector_append::operator()(compile_error &e0, compile_error &e1) const { return merge_errors<compile_error>(e0, e1); }*/

codegen_void errors::merge_void_values(codegen_void &v0, codegen_void &v1) {
  typedef argument_value_join<codegen_void, codegen_void>::result_value_type merged_type;
  boost::function<codegen_void (merged_type &)> op([] (merged_type &) -> codegen_void { return nullptr; });
  return codegen_call_args<codegen_void, codegen_void, codegen_void>(op, v0, v1);
}
