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

#ifndef RT_CAMERA_HPP
#define RT_CAMERA_HPP

#include "math/vector.hpp"
#include "math/transform.hpp"
#include "geometry/ray.hpp"

namespace raytrace {

  /* Description of a scene's viewing camera. */
  struct camera {
    float clip_start, clip_end;
    
    transform raster_to_camera;
    transform camera_to_world;
  };

  /* Creates a ray shooting from the given screen-space location. */
  ray camera_shoot_ray(const camera &cam, float x, float y);
  
};

#endif
