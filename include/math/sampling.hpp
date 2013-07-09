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

#ifndef RT_SAMPLING_HPP
#define RT_SAMPLING_HPP

#include "math/vector.hpp"

namespace raytrace {

  const float pi = 3.14159265359f;

  /* Disk sampling methods taken from Physically-Based Rendering */
  
  float2 sample_unit_disk(float rand_u, float rand_v);
  float2 concentric_sample_unit_disk(float rand_u, float rand_v);
  
  float3 cosine_sample_hemisphere(const float3 &N, float rand_u, float rand_v);

  float3 uniform_sample_sphere(float rand_u, float rand_v);
};

#endif
