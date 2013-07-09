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

#ifndef RT_SCENE_HPP
#define RT_SCENE_HPP

#include <vector>

#include "math/vector.hpp"
#include "scene/primitive.hpp"
#include "scene/object.hpp"
#include "scene/camera.hpp"
#include "scene/light.hpp"

#include "shading/distribution.hpp"

namespace raytrace {

  /* Holds all geometry data for a scene. */
  struct scene {
    //clears all primitives, objects and lights in this scene
    void clear();

    //camera
    camera main_camera;
    int2 resolution;
    
    //mesh geometry data
    std::vector<float3> vertices, vertex_normals;
    std::vector<int3> triangle_verts;
    
    //primitive list
    std::vector<primitive> primitives;
    std::vector<object_ptr> objects;

    //lights
    std::vector<light> lights;
  };

};

#endif
