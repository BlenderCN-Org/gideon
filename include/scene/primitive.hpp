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
       shader_id - Pointer to the primitive's material function
  */
  struct primitive {
    enum { PRIM_TRIANGLE, PRIM_STRAND } type;
    int id;
    int data_id;
    int object_id, instance_id;
    void *shader_id;
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
