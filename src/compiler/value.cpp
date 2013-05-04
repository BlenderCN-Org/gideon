#include "compiler/value.hpp"

#include "compiler/lookup.hpp"

using namespace raytrace;
using namespace llvm;
using namespace std;

value::value(Value *v) : data(v) { }
value::value(const module_ptr &module) : data(module) { }

template<typename T>
class value_extractor : public boost::static_visitor<T> {
public:

  template<typename U>
  T operator()(U &data) const { throw runtime_error("Extraction on invalid type!"); }
  T operator()(T &data) const { return data; }

};

Value *value::extract_value() {
  return apply_visitor<Value*>(value_extractor<Value*>());
}

value::module_ptr value::extract_module() {
  return apply_visitor<module_ptr>(value_extractor<module_ptr>());
}

value::value_type value::type() const {
  int which = data.which();
  if (which == 0) return LLVM_VALUE;
  return MODULE_VALUE;
}
