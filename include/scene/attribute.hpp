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

#ifndef RT_ATTRIBUTE_HPP
#define RT_ATTRIBUTE_HPP

#include <string>
#include <vector>

#include "math/vector.hpp"

namespace raytrace {

  /* Description of a data type that can be an attribute. */
  struct attribute_type {
    enum { FLOAT, INT, LASTBASE } base_type;
    enum { SCALAR=1, VEC2=2, VEC3=3, VEC4=4 } aggregate_type;
    size_t size() const;

    bool operator==(const attribute_type &rhs) const;
    bool operator!=(const attribute_type &rhs) const { return !(*this == rhs); }
  };

  /* Allows one to get a C++ type's equivalent attributes. */
  template<typename T>
  attribute_type get_attribute_type();

  class attribute {
  public:
    typedef enum { PER_OBJECT, PER_VERTEX, PER_PRIMITIVE, PER_CORNER } element_type;
    
    element_type element;
    attribute_type type;

    attribute(const element_type &e, const attribute_type &t);

    //Resize the buffer to hold enough data for N elements.
    void resize(unsigned int N);

    //Returns a pointer to the data for the i-th element.
    template<typename T>
    const T *data(int i) const { return reinterpret_cast<T*>(&buffer[i*items_per_element()*type.size()]); }

    template<typename T>
    T *data(int i) { return reinterpret_cast<T*>(&buffer[i*items_per_element()*type.size()]); }

  private:
    
    std::vector<char> buffer;
    size_t items_per_element() const;
    
  };

};

#endif
