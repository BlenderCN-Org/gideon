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
