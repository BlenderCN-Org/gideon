#include "math/sampling.hpp"

#include <math.h>

using namespace raytrace;

float2 raytrace::sample_unit_disk(float rand_u, float rand_v) {
  float r = sqrtf(rand_u);
  float theta = 2.0f * pi * rand_v;

  return {r * cosf(theta), r * sinf(theta)};
}
