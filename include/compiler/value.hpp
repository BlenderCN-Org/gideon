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

#ifndef GD_RL_VALUE_HPP
#define GD_RL_VALUE_HPP

#include <memory>
#include <boost/variant.hpp>
#include "llvm/IR/DerivedTypes.h"

namespace raytrace {

  class module_object;

  class value {
  public:
    
    typedef std::shared_ptr<module_object> module_ptr;

    value(llvm::Value *v);
    value(const module_ptr &module);

    llvm::Value *extract_value();
    module_ptr extract_module();

    template<typename T, typename Visitor>
    T apply_visitor(const Visitor &visitor) {
      return boost::apply_visitor(visitor, data);
    }

    typedef enum { LLVM_VALUE, MODULE_VALUE } value_type;
    value_type type() const;
    
  private:

    boost::variant<llvm::Value*, module_ptr> data;

  };

};


#endif
