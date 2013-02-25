#ifndef RT_SAMPLING_HPP
#define RT_SAMPLING_HPP

#include "math/vector.hpp"

namespace raytrace {

  const float pi = 3.14159265359f;

  float2 sample_unit_disk(float rand_u, float rand_v);

};

#endif
