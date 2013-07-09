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
