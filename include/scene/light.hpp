#ifndef RT_LIGHT_HPP
#define RT_LIGHT_HPP

#include "math/vector.hpp"

namespace raytrace {
  
  struct point_light_data {
    float3 position;
    float radius;
  };

  struct light {
    enum { POINT } type;
    union {
      point_light_data point;
    };
  
    float energy;
    float3 color;

    /*
      Samples a random position on the surface of a light. If the w component is zero, a direction is specified (in case of a directional light). 
      Point to be illuminated is provided to do view-dependent sampling.
    */
    float4 sample_position(const float3 &P, float rand_u, float rand_v) const;
    
    /* Evaluates the light emitted from a point on a light in a given direction. */
    float3 eval_radiance(const float3 &P, const float3 &I) const;
  };


};

#endif
