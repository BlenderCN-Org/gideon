#ifndef RT_RAY_HPP
#define RT_RAY_HPP

#include "math/vector.hpp"

namespace raytrace {

  /* Ray Types */

  struct ray {
    float3 o, d;
    float min_t, max_t;

    float3 point_on_ray(float t) { return o + t*d; }
  };

  struct intersection {
    float t, u, v;
    int prim_idx;
  };
  
};

#endif
