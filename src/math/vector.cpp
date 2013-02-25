#include <math.h>
#include "math/vector.hpp"

namespace raytrace {

  float length(const float2 &v) {
    return sqrtf(v.x*v.x + v.y*v.y);
  }
  
  float length(const float3 &v) {
    return sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
  }
  
  float length(const float4 &v) {
    return sqrtf(v.x*v.x + v.y*v.y + v.z*v.z + v.w*v.w);
  }
  
  float2 normalize(const float2 &v) { return (1.0f / length(v)) * v; }
  float3 normalize(const float3 &v) { return (1.0f / length(v)) * v; }
  float4 normalize(const float4 &v) { return (1.0f / length(v)) * v; }
  
  float3 cross(const float3 &a, const float3 &b) {
    return {a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x};
  }

  void make_orthonormals(const float3 &N,
			 /* out */ float3 &T, /* out */ float3 &B) {
    if (N.x != N.y || N.x != N.z)
      T = float3{N.z - N.y, N.x - N.z, N.y - N.x}; // (1, 1, 1) x N
    else 
      T = float3{N.z - N.y, N.x + N.z, -N.y - N.x}; // (-1, 1, 1) x N

    T = normalize(T);
    B = cross(N, T);
  }

};
