#ifndef RT_SAMPLING_HPP
#define RT_SAMPLING_HPP

#include "math/vector.hpp"

namespace raytrace {

  const float pi = 3.14159265359f;

  /* Disk sampling methods taken from Physically-Based Rendering */
  
  float2 sample_unit_disk(float rand_u, float rand_v);
  float2 concentric_sample_unit_disk(float rand_u, float rand_v);
  
  float3 cosine_sample_hemisphere(const float3 &N, float rand_u, float rand_v);

};

#endif
