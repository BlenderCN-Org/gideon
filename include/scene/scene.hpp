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
