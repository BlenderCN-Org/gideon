#ifndef RT_TRIANGLE_HPP
#define RT_TRIANGLE_HPP

#include "math/vector.hpp"
#include "geometry/ray.hpp"
#include "geometry/aabb.hpp"

namespace raytrace {
  
  /* Ray Intersection */

  bool ray_triangle_intersection(const float3 &v0, const float3 &v1, const float3 &v2,
				 const ray &r, /* out */ intersection &isect);

  /* Bounding Box */
  
  aabb compute_triangle_bbox(const float3 &v0, const float3 &v1, const float3 &v2);

  /* Triangle Differential Geometry */

  float3 compute_triangle_normal(const float3 &v0, const float3 &v1, const float3 &v2);

  //Computes the derivatives of the triangle in Barycentric coordinates.
  void compute_triangle_dP(const float3 &v0, const float3 &v1, const float3 &v2,
			   /* out */ float3 &dPdu, /* out */ float3 &dPdv);
};

#endif
