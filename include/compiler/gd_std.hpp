#ifndef GD_STD_OPS_HPP
#define GD_STD_OPS_HPP

#include "scene/scene.hpp"
#include "scene/bvh.hpp"
#include "geometry/ray.hpp"

namespace gideon {

  namespace rl {
    
    //Data that the built-in functions expect to be associated with the scene pointer.
    struct scene_data {
      raytrace::scene *s;
      raytrace::bvh *accel;
    };

  };
  
};

#endif
