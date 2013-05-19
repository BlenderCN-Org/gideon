#include "compiler/errors.hpp"
#include <sstream>

using namespace raytrace;
using namespace llvm;
using namespace std;

namespace raytrace {
  namespace errors {
    template<>
    compile_error merge_errors<compile_error>(const compile_error &e0, const compile_error &e1) {
      return make_error<error_pair>(e0.e, e1.e);
    }
  };
};



Value *errors::codegen_value_extract(codegen_value &val) {
  return boost::apply_visitor(errors::return_or_throw<Value*>(), val);
}

codegen_void errors::codegen_ignore_value(codegen_value &val) {
  auto op = [] (Value *&) -> codegen_void {
    return empty_type();
  };
  raytrace::errors::value_container_operation<codegen_value, codegen_void> ignore(op);
  return boost::apply_visitor(ignore, val);
}

codegen_void errors::merge_void::operator()(compile_error &e0, compile_error &e1) const { return merge_errors<compile_error>(e0, e1); }

/*codegen_vector errors::codegen_vector_append::operator()(vector<Value*> &list, Value *&val) const {
  vector<Value*> result = list;
  result.push_back(val);
  return result;
}

codegen_vector errors::codegen_vector_append::operator()(compile_error &e0, compile_error &e1) const { return merge_errors<compile_error>(e0, e1); }*/

codegen_void errors::merge_void_values(codegen_void &v0, codegen_void &v1) {
  typedef argument_value_join<codegen_void, codegen_void>::result_value_type merged_type;
  boost::function<codegen_void (merged_type &)> op([] (merged_type &) -> codegen_void { return empty_type(); });
  return codegen_call_args<codegen_void, codegen_void, codegen_void>(op, v0, v1);
}

/** Error Classes **/

/* Error Base */
errors::error::error(unsigned int line_no, unsigned int column_no) :
  line_no(line_no), column_no(column_no)
{

}

void errors::error::set_location(unsigned int line, unsigned int column) {
  line_no = line;
  column_no = column;
}

string errors::error::location() const {
  stringstream ss;
  ss << "Line " << line_no << ":" << column_no;
  return ss.str();
}


/* Error Pair */

errors::error_pair::error_pair(const error_ptr &first, const error_ptr &second) :
  error(0, 0),
  first(first), second(second)
{

}

string errors::error_pair::report() const {
  stringstream ss;
  ss << first->report() << endl << second->report();
  return ss.str();
}

/* Error Group */

errors::error_group::error_group(const string &group_name,
				 const error_ptr &errors,
				 unsigned int line_no, unsigned int column_no) :
  error(line_no, column_no),
  group_name(group_name),
  errors(errors)
{

}

string errors::error_group::report() const {
  stringstream ss;
  ss << location() << " - In " << group_name << ":" << endl;
  ss << errors->report();
  
  return ss.str();
}

/* Error Message */

errors::error_message::error_message(const string &msg,
				     unsigned int line_no, unsigned int column_no) :
  error(line_no, column_no),
  msg(msg)
{

}

string errors::error_message::report() const {
  stringstream ss;
  ss << location() << " - " << msg;
  return ss.str();
}

/* Undefined Variable */

errors::undefined_variable::undefined_variable(const string &name,
					       unsigned int line_no, unsigned int column_no) :
  error(line_no, column_no),
  name(name)
{

}

string errors::undefined_variable::report() const {
  stringstream ss;
  ss << location() << " - Undefined variable '" << name << "'.";
  return ss.str();
}

/* Invalid Function Call */

errors::invalid_function_call::invalid_function_call(const string &name, const string &error_msg,
						     unsigned int line_no, unsigned int column_no) :
  error(line_no, column_no),
  name(name), error_msg(error_msg)
{
  
}

string errors::invalid_function_call::report() const {
  stringstream ss;
  ss << location() << " - Invalid function call '" << name << "': " << error_msg;
  return ss.str();
}

/* Type Mismatch */

errors::type_mismatch::type_mismatch(const string &type, const string &expected,
				     unsigned int line_no, unsigned int column_no) :
  error(line_no, column_no),
  type(type), expected(expected)
{

}

string errors::type_mismatch::report() const {
  stringstream ss;
  ss << location() << " - Type Mismatch: Found " << type;
  ss << " expected " << expected;

  return ss.str();
}
  
