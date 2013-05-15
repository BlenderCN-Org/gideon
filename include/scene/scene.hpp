#ifndef RT_SCENE_HPP
#define RT_SCENE_HPP

#include <vector>

#include "math/vector.hpp"
#include "scene/primitive.hpp"
#include "scene/object.hpp"
#include "scene/camera.hpp"
#include "scene/light.hpp"

#include "shading/distribution.hpp"
#include "vm/program.hpp"

namespace raytrace {

  /* Holds all geometry data for a scene. */
  struct scene {
    ~scene();

    //camera
    camera main_camera;
    int2 resolution;
    
    //mesh geometry data
    std::vector<float3> vertices, vertex_normals;
    std::vector<int3> triangle_verts;
    
    //primitive list
    std::vector<primitive> primitives;
    std::vector<object*> objects;

    //lights
    std::vector<light> lights;

    //reflectance distribution functions
    std::vector<distribution_function> distributions;

    //list of programs used by the scene's shaders and distributions
    std::vector<program> programs;
  };

};

#endif
