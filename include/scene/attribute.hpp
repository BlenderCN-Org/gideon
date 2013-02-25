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
