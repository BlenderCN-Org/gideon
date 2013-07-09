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

#include "scene/camera.hpp"

using namespace raytrace;

ray raytrace::camera_shoot_ray(const camera &cam, float x, float y) {
  float3 raster{x, y, 0.0f};

  ray r;
  r.o = cam.camera_to_world.apply_point({0.0f, 0.0f, 0.0f});
  r.d = cam.camera_to_world.apply_direction(normalize(cam.raster_to_camera.apply_perspective(raster)));
  r.min_t = cam.clip_start;
  r.max_t = cam.clip_end;

  return r;
}
