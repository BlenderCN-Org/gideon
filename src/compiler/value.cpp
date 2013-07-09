/*

  Copyright 2013 Curtis Andrus

  This file is part of Gideon.

  Gideon is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  Gideon is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with Gideon.  If not, see <http://www.gnu.org/licenses/>.

*/

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
