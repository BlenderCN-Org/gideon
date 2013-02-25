#ifndef RT_AABB_HPP
#define RT_AABB_HPP

#include "math/vector.hpp"
#include "geometry/ray.hpp"

namespace raytrace {
  
  /* AABB Type */

  struct aabb {
    float3 pmin, pmax;

    float volume() const;
    float surfacearea() const;
    float3 center() const;

    aabb merge(const aabb &rhs) const;
    
    static aabb empty_box();
  };
  
  /* Ray Intersection */
  
  bool ray_aabb_intersection(const aabb &box, const ray &r,
			     /* out */ float &t0, /* out */ float &t1);
  
};

#endif
