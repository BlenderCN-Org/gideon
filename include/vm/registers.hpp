#ifndef RT_REGISTERS_HPP
#define RT_REGISTERS_HPP

#include <string>
#include <vector>
#include <memory>

#include "math/vector.hpp"
#include "geometry/ray.hpp"

namespace raytrace {

  struct object;
  struct primitive;
  struct shader_node;
  
  typedef std::shared_ptr<shader_node> shader_ref;

  struct register_file {
    register_file(unsigned int size) :
      f_data(size), f2_data(size), f3_data(size), f4_data(size),
      i_data(size), i2_data(size), i3_data(size), i4_data(size),
      str_data(size), ray_data(size), isect_data(size),
      obj_data(4), prim_data(4), ptr_data(8), shader_data(8)
    {}

    template<typename T>
    T &get(int i);

    template<typename T>
    size_t capacity() const;

    std::vector<float> f_data;
    std::vector<float2> f2_data;
    std::vector<float3> f3_data;
    std::vector<float4> f4_data;

    std::vector<int> i_data;
    std::vector<int2> i2_data;
    std::vector<int3> i3_data;
    std::vector<int4> i4_data;

    std::vector<std::string> str_data;

    std::vector<ray> ray_data;
    std::vector<intersection> isect_data;

    std::vector<object*> obj_data;
    std::vector<primitive*> prim_data;

    std::vector<void*> ptr_data;
    std::vector<shader_ref> shader_data;
  };

};

#endif
