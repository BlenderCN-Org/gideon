#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/JIT.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/PassManager.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/DataLayout.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Attributes.h"

#include "math/vector.hpp"

#include "compiler/rendermodule.hpp"

#include <fstream>
#include <iostream>

using namespace std;
using namespace raytrace;
using namespace llvm;

template<typename...>
struct tuple_join;

template<typename T0>
struct tuple_join<T0> {
  typedef boost::tuples::cons<T0, boost::tuples::null_type> type;

  static type make_tuple(const T0 &t) { return t; }
};

template<typename T0, typename... TTail>
struct tuple_join<T0, TTail...> {

  typedef boost::tuples::cons<T0, typename tuple_join<TTail...>::type> type;

  static type make_tuple(const T0 &t, const TTail &... tail) { return type(t, tuple_join<TTail...>::make_tuple(tail...)); }
};

template<typename... Types>
typename tuple_join<Types...>::type build_tuple(const Types &... vals) { return tuple_join<Types...>::make_tuple(vals...); }

void test_tuple_stuff() {
  
  auto x = build_tuple(4.0f);
  auto y = build_tuple(4.0f, -45);
  
  typedef boost::variant<int, compile_error> val_type;
  val_type my_val = 5;

  auto z = errors::argument_value_join< val_type>::build(my_val);
  auto w = errors::argument_value_join<val_type, val_type>::build(my_val, my_val);
}

extern "C" void rtl_normalize_v4(float4 *result, float4 *v) {
  *result = normalize(*v);
}

int main(int argc, char **argv) {
  InitializeNativeTarget();
  
  ifstream render_file("/home/curtis/Projects/relatively-crazy/tests/render_loop.gdl");
  string source((istreambuf_iterator<char>(render_file)), istreambuf_iterator<char>());
  cout << "Source Code: " << endl << source << endl;
  render_module render2("test_loop", source);
  Module *module2 = render2.compile();
  module2->dump();
  verifyModule(*module2);
  
  //delete engine;
  return 0;
}
