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

#include "math/transform.hpp"

using namespace raytrace;

transform transform::operator*(const transform &rhs) const {
  transform result;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      float4 col{rhs.rows[0][j], rhs.rows[1][j], rhs.rows[2][j], rhs.rows[3][j]};

      result.rows[i][j] = dot(rows[i], col);
    }
  }

  return result;
}

float4 transform::apply(const float4 &v) const {
  return {dot(rows[0], v), dot(rows[1], v), dot(rows[2], v), dot(rows[3], v)};
}

float3 transform::apply_point(const float3 &v) const {
  float4 v4{v.x, v.y, v.z, 1.0f};
  return {dot(rows[0], v4), dot(rows[1], v4), dot(rows[2], v4)};
}

float3 transform::apply_perspective(const float3 &v) const {
  float4 t = apply({v.x, v.y, v.z, 1.0f});
  float inv_w = 1.0f / t.w;
  return inv_w * float3{t.x, t.y, t.z};
}

float3 transform::apply_direction(const float3 &v) const {
  float4 t = apply({v.x, v.y, v.z, 0.0f});
  return {t.x, t.y, t.z};
}
