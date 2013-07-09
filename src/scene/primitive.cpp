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

#include "scene/primitive.hpp"
#include "scene/scene.hpp"
#include "geometry/ray.hpp"
#include "geometry/triangle.hpp"
#include "geometry/aabb.hpp"

using namespace std;
using namespace raytrace;

bool raytrace::ray_primitive_intersection(const primitive &prim, const scene &active_scene,
					  const ray &r, /* out */ intersection &isect) {
  if (prim.type == primitive::PRIM_TRIANGLE) {
    const int3 &tri_verts = active_scene.triangle_verts[prim.data_id];
    return ray_triangle_intersection(active_scene.vertices[tri_verts.x],
				     active_scene.vertices[tri_verts.y],
				     active_scene.vertices[tri_verts.z],
				     r, isect);
  }
  
  return false;
}

aabb raytrace::primitive_bbox(const primitive &prim, const scene &active_scene) {
  if (prim.type == primitive::PRIM_TRIANGLE) {
    const int3 &tri_verts = active_scene.triangle_verts[prim.data_id];
    return compute_triangle_bbox(active_scene.vertices[tri_verts.x],
				 active_scene.vertices[tri_verts.y],
				 active_scene.vertices[tri_verts.z]);
  }
  
  return aabb();
}

float3 raytrace::primitive_geometry_normal(const primitive &prim, const scene &active_scene) {
  if (prim.type == primitive::PRIM_TRIANGLE) {
    const int3 &tri_verts = active_scene.triangle_verts[prim.data_id];
    return compute_triangle_normal(active_scene.vertices[tri_verts.x],
				   active_scene.vertices[tri_verts.y],
				   active_scene.vertices[tri_verts.z]);
  }
  else return {0.0f, 0.0f, 0.0f};
}

int3 raytrace::primitive_get_attribute_id_per_vertex(const primitive &prim, const scene &active_scene) {
  object_ptr obj = active_scene.objects[prim.object_id];
  
  if (prim.type == primitive::PRIM_TRIANGLE) {
    int3 verts = active_scene.triangle_verts[prim.data_id];
    int offset = obj->vert_range.x;
    
    return { verts.x - offset, verts.y - offset, verts.z - offset};	
  }
  else return {0, 0, 0};
}

int raytrace::primitive_get_attribute_id_per_primitive(const primitive &prim, const scene &active_scene) {
  object_ptr obj = active_scene.objects[prim.object_id]; 
  return prim.id - obj->prim_range.x;
}

int raytrace::primitive_get_attribute_id_per_corner(const primitive &prim, const scene &active_scene) {
  object_ptr obj = active_scene.objects[prim.object_id]; 
  
  if (prim.type == primitive::PRIM_TRIANGLE) return prim.data_id - obj->tri_range.x;
  else return 0;
}
