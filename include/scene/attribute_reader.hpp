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

#ifndef RT_ATTRIBUTE_READER_HPP
#define RT_ATTRIBUTE_READER_HPP

#include "scene/scene.hpp"

namespace raytrace {

  /* Reads a per-vertex attribute over the surface of a triangle. */
  template<typename T>
  T triangle_get_attribute(attribute *attr,
			   const primitive &prim, const object_ptr &obj,
			   const scene &active_scene,
			   const float4 &coords) {
    T *c0, *c1, *c2;

    if (attr->element == attribute::PER_VERTEX) {
      const int3 &verts = active_scene.triangle_verts[prim.data_id];
      c0 = attr->data<T>(verts.x - obj->vert_range.x);
      c1 = attr->data<T>(verts.y - obj->vert_range.x);
      c2 = attr->data<T>(verts.z - obj->vert_range.x);
    }
    else {
      T *val = attr->data<T>(prim.data_id - obj->tri_range.x);
      c0 = val;
      c1 = val + 1;
      c2 = val + 2;
    }

    float inv = 1.0f - coords[0] - coords[1];
    return coords[0]*(*c1) + coords[1]*(*c2) + inv*(*c0);
  }

  /* Same as normal get_attribute, except this includes derivatives. */
  template<typename T>
  T triangle_get_attribute_deriv(attribute *attr,
				 const primitive &prim, const object_ptr &obj,
				 const scene &active_scene,
				 const float4 &coords,
				 /* out */ T &du, /* out */ T &dv) {
    T *c0, *c1, *c2;

    if (attr->element == attribute::PER_VERTEX) {
      const int3 &verts = active_scene.triangle_verts[prim.data_id];
      c0 = attr->data<T>(verts.x - obj->vert_range.x);
      c1 = attr->data<T>(verts.y - obj->vert_range.x);
      c2 = attr->data<T>(verts.z - obj->vert_range.x);
    }
    else {
      T *val = attr->data<T>(prim.data_id - obj->tri_range.x);
      c0 = val;
      c1 = val + 1;
      c2 = val + 2;
    }

    du = *c1 - *c0;
    dv = *c2 - *c0;

    float inv = 1.0f - coords[0] - coords[1];
    return coords[0]*(*c1) + coords[1]*(*c2) + inv*(*c0);
  }
  
  /* Reads an attribute from a primitive. Returns false is no valid attribute can be found. */
  template<typename T>
  bool primitive_get_attribute(const primitive &prim, const scene &active_scene,
			       const std::string &attr_name, const float4 &coords,
			       /* out */ T &result) {
    object_ptr obj = active_scene.objects[prim.object_id];

    auto attr_it = obj->attributes.find(attr_name);
    if (attr_it == obj->attributes.end()) return false; //no attribute with this name
    
    attribute *attr = attr_it->second;
    if (get_attribute_type<T>() != attr->type) return false; //type mismatch

    if (attr->element == attribute::PER_OBJECT) result = *(attr->data<T>(0));
    else if (attr->element == attribute::PER_PRIMITIVE) result = *(attr->data<T>(prim.id - obj->prim_range.x));
    else if (prim.type == primitive::PRIM_TRIANGLE) result = triangle_get_attribute<T>(attr, prim, obj, active_scene, coords);
    
    return true;
  }

  template<typename T>
  bool primitive_get_attribute_deriv(const primitive &prim, const scene &active_scene,
				     const std::string &attr_name, const float4 &coords,
				     /* out */ T &result, /* out */ T &du, /* out */ T &dv) {
    object_ptr obj = active_scene.objects[prim.object_id];
    
    auto attr_it = obj->attributes.find(attr_name);
    if (attr_it == obj->attributes.end()) return false; //no attribute with this name
    
    attribute *attr = attr_it->second;
    if (get_attribute_type<T>() != attr->type) return false; //type mismatch

    if (attr->element == attribute::PER_OBJECT) result = *(attr->data<T>(0));
    else if (attr->element == attribute::PER_PRIMITIVE) result = *(attr->data<T>(prim.id - obj->prim_range.x));
    else if (prim.type == primitive::PRIM_TRIANGLE) result = triangle_get_attribute_deriv<T>(attr, prim, obj, active_scene, coords, du, dv);
    
    return true;
  }

};

#endif
