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

#include "math/sampling.hpp"

#include <algorithm>
#include <math.h>

using namespace raytrace;

float2 raytrace::sample_unit_disk(float rand_u, float rand_v) {
  float r = sqrtf(rand_u);
  float theta = 2.0f * pi * rand_v;

  return {r * cosf(theta), r * sinf(theta)};
}

float2 raytrace::concentric_sample_unit_disk(float rand_u, float rand_v) {
  float r, theta;
  float sx = 2.0f * rand_u - 1.0f;
  float sy = 2.0f * rand_v - 1.0f;

  //map the square to (r, theta)
  if (sx == 0.0 && sy == 0.0) return { 0.0f, 0.0f };
  
  if (sx >= -sy) {
    if (sx > sy) {
      //first region of disk
      r = sx;
      if (sy > 0.0f) theta = sy / r;
      else theta = 8.0f + (sy / r);
    
    }
    else {
      //second region
      r = sy;
      theta = 2.0f - (sx / r);
    }
  }
  else {
    if (sx <= sy) {
      //third region
      r = -sx;
      theta = 4.0f - (sy / r);
    }
    else {
      //fourth region
      r = -sy;
      theta = 6.0f + (sx / r);
    }
  }

  theta *= (pi * 0.25f);
  return {r * cosf(theta), r * sinf(theta)};
}

float3 raytrace::cosine_sample_hemisphere(const float3 &N, float rand_u, float rand_v) {
  float2 v = concentric_sample_unit_disk(rand_u, rand_v);
  float z = sqrtf(std::max(0.0f, 1.0f - (v.x*v.x) - (v.y*v.y)));

  float3 T, B;
  make_orthonormals(N, T, B);
  return (v.x * T) + (v.y * B) + (z * N);
}

float3 raytrace::uniform_sample_sphere(float rand_u, float rand_v) {
  float z = 2.0f * (rand_u - 1.0f);
  float t = rand_v * 2.0f * pi;
  
  float r = sqrtf(1.0f - (z*z));
  return {r*cosf(t), r*sinf(t), z};
}
