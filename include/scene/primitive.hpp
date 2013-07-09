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

#ifndef RT_PRIMITIVE_HPP
#define RT_PRIMITIVE_HPP

#include "math/vector.hpp"

namespace raytrace {

  struct scene;
  struct ray;
  struct intersection;
  struct aabb;
  struct attribute;
  
  /* 
     Reference to a single primitive in the scene:
       type - The type of this primitive
       id - Index into the scene's primitive list
       data_id - Index into the scene's array of primitives of these types
       object_id - Index of the object containing this primitive
       instance_id - Index of this object's instance, if applicable (-1 otherwise)
       shader_id - Pointer to the primitive's surface material function
       volume_id - Pointer to the primitive's surface volume function
  */
  struct primitive {
    enum { PRIM_TRIANGLE, PRIM_STRAND } type;
    int id;
    int data_id;
    int object_id, instance_id;
    void *shader_id, *volume_id;
  };
  
  bool ray_primitive_intersection(const primitive &prim, const scene &active_scene,
				  const ray &r, /* out */ intersection &isect);

  aabb primitive_bbox(const primitive &prim, const scene &active_scene);

  float3 primitive_geometry_normal(const primitive &prim, const scene &active_scene);

  int3 primitive_get_attribute_id_per_vertex(const primitive &prim, const scene &active_scene);
  int primitive_get_attribute_id_per_primitive(const primitive &prim, const scene &active_scene);
  int primitive_get_attribute_id_per_corner(const primitive &prim, const scene &active_scene);
  
};

#endif
