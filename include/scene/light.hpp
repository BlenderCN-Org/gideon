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

#ifndef RT_LIGHT_HPP
#define RT_LIGHT_HPP

#include "math/vector.hpp"

namespace raytrace {
  
  struct point_light_data {
    float3 position;
    float radius;
  };

  struct light {
    enum { POINT } type;
    union {
      point_light_data point;
    };
  
    float energy;
    float3 color;

    /*
      Samples a random position on the surface of a light. If the w component is zero, a direction is specified (in case of a directional light). 
      Point to be illuminated is provided to do view-dependent sampling.
    */
    float4 sample_position(const float3 &P, float rand_u, float rand_v,
			   /* out */ float &pdf) const;
    
    /* Evaluates the light emitted from a point on a light in a given direction. */
    float4 eval_radiance(const float3 &P, const float3 &I) const;
  };


};

#endif
