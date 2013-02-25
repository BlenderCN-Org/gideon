#ifndef RT_TRANSFORM_HPP
#define RT_TRANSFORM_HPP

#include "math/vector.hpp"

namespace raytrace {

  /* Represents a mathematical transformation (rigid body, perspective) */
  struct transform {
    float4 rows[4];

    transform operator*(const transform &rhs) const;
    
    float4 apply(const float4 &v) const;
    
    float3 apply_point(const float3 &v) const;
    float3 apply_perspective(const float3 &v) const;
    float3 apply_direction(const float3 &v) const;
  };
  
};

#endif
