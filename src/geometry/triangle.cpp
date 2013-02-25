#include "geometry/triangle.hpp"
#include "math/util.hpp"

using namespace raytrace;

bool raytrace::ray_triangle_intersection(const float3 &v0, const float3 &v1, const float3 &v2,
					 const ray &r, /* out */ intersection &isect) {
  float3 edge1 = v1 - v0;
  float3 edge2 = v2 - v0;
  float3 pvec = cross(r.d, edge2);

  float det = dot(edge1, pvec);

  if (det > -epsilon && det < epsilon) return false;
  float inv_det = 1.0f / det;

  float3 tvec = r.o - v0;
  isect.u = dot(tvec, pvec) * inv_det;
  if (isect.u < 0.0f || isect.u > 1.0f) return false;
  
  float3 qvec = cross(tvec, edge1);
  isect.v = dot(r.d, qvec) * inv_det;
  if (isect.v < 0.0f || (isect.u + isect.v) > 1.0f) return false;

  isect.t = dot(edge2, qvec) * inv_det;
  if (isect.t < r.min_t || isect.t > r.max_t) return false;
  return true;
}

aabb raytrace::compute_triangle_bbox(const float3 &v0, const float3 &v1, const float3 &v2) {
  aabb result;
  minmax3(v0.x, v1.x, v2.x, result.pmin.x, result.pmax.x);
  minmax3(v0.y, v1.y, v2.y, result.pmin.y, result.pmax.y);
  minmax3(v0.z, v1.z, v2.z, result.pmin.z, result.pmax.z);
  return result;
}

float3 raytrace::compute_triangle_normal(const float3 &v0, const float3 &v1, const float3 &v2) {
  return normalize(cross(v1 - v0, v2 - v0));
}

void raytrace::compute_triangle_dP(const float3 &v0, const float3 &v1, const float3 &v2,
				   /* out */ float3 &dPdu, /* out */ float3 &dPdv) {
  dPdu = (v1 - v0);
  dPdv = (v2 - v0);
}
