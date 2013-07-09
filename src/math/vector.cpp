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

#include <math.h>
#include "math/vector.hpp"

namespace raytrace {

  float length(const float2 &v) {
    return sqrtf(v.x*v.x + v.y*v.y);
  }
  
  float length(const float3 &v) {
    return sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
  }
  
  float length(const float4 &v) {
    return sqrtf(v.x*v.x + v.y*v.y + v.z*v.z + v.w*v.w);
  }
  
  float2 normalize(const float2 &v) { return (1.0f / length(v)) * v; }
  float3 normalize(const float3 &v) { return (1.0f / length(v)) * v; }
  float4 normalize(const float4 &v) { return (1.0f / length(v)) * v; }
  
  float3 cross(const float3 &a, const float3 &b) {
    return {a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x};
  }

  void make_orthonormals(const float3 &N,
			 /* out */ float3 &T, /* out */ float3 &B) {
    if (N.x != N.y || N.x != N.z)
      T = float3{N.z - N.y, N.x - N.z, N.y - N.x}; // (1, 1, 1) x N
    else 
      T = float3{N.z - N.y, N.x + N.z, -N.y - N.x}; // (-1, 1, 1) x N

    T = normalize(T);
    B = cross(N, T);
  }

  float3 spherical_direction(const float3 &N, const float3 &T, const float3 &B,
			     float sin_theta, float cos_theta, float phi) {
    return sin_theta * cosf(phi) * T + sin_theta * sinf(phi) * B + cos_theta * N;
  }

};
