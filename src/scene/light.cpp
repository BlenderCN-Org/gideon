#include "scene/light.hpp"
#include "math/sampling.hpp"

using namespace raytrace;

float4 raytrace::light::sample_position(const float3 &P, float rand_u, float rand_v,
					/* out */ float &pdf) const {
  if (type == POINT) {
    float3 I = normalize(point.position - P);
    float3 T, B;
    make_orthonormals(I, T, B);

    //float2 disk_sample = point.radius * sample_unit_disk(rand_u, rand_v);
    float2 disk_sample = point.radius * concentric_sample_unit_disk(rand_u, rand_v);
    pdf = 1.0f / (pi * point.radius * point.radius);
    
    float3 LP = point.position + disk_sample.x*T + disk_sample.y*B;
    return float4{LP.x, LP.y, LP.z, 1.0f};
  }

  return {0.0f, 0.0f, 0.0f, 1.0f};
}

float4 raytrace::light::eval_radiance(const float3 &P, const float3 &I) const {
  if (type == POINT) {
    float area = pi * point.radius * point.radius;
    float3 R = (energy / area)*color;
    return {R.x, R.y, R.z, 1.0f};
  }

  return {0.0f, 0.0f, 0.0f};
}
