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
