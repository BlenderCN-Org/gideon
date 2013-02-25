#include "scene/attribute.hpp"

using namespace std;
using namespace raytrace;

static const size_t typesize_array[] = {
  sizeof(float),
  sizeof(int),

  sizeof(float2),
  sizeof(int2),

  sizeof(float3),
  sizeof(int3),

  sizeof(float4),
  sizeof(int4),
};

bool raytrace::attribute_type::operator==(const attribute_type &rhs) const {
  return (base_type == rhs.base_type) && (aggregate_type == rhs.aggregate_type);
}

size_t raytrace::attribute_type::size() const {
  return typesize_array[LASTBASE * aggregate_type + base_type];
}

namespace raytrace {

  template<> attribute_type get_attribute_type<int>() {return { attribute_type::INT, attribute_type::SCALAR}; }
  template<> attribute_type get_attribute_type<float>() {return { attribute_type::FLOAT, attribute_type::SCALAR}; }

  template<> attribute_type get_attribute_type<int2>() {return { attribute_type::INT, attribute_type::VEC2}; }
  template<> attribute_type get_attribute_type<float2>() {return { attribute_type::FLOAT, attribute_type::VEC2}; }

  template<> attribute_type get_attribute_type<int3>() {return { attribute_type::INT, attribute_type::VEC3}; }
  template<> attribute_type get_attribute_type<float3>() {return { attribute_type::FLOAT, attribute_type::VEC3}; }

  template<> attribute_type get_attribute_type<int4>() {return { attribute_type::INT, attribute_type::VEC4}; }
  template<> attribute_type get_attribute_type<float4>() {return { attribute_type::FLOAT, attribute_type::VEC4}; }
  
};

/** attribute implementation **/

raytrace::attribute::attribute(const element_type &e, const attribute_type &t) :
  element(e), type(t)
{
  
}

size_t raytrace::attribute::items_per_element() const {
  switch (element) {
  case PER_OBJECT:
  case PER_VERTEX:
  case PER_PRIMITIVE:
    return 1;
  case PER_CORNER:
    return 3;
  }
}

void raytrace::attribute::resize(unsigned int N) {
  buffer.resize(items_per_element()*type.size()*N);
}

