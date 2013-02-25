#include "geometry/aabb.hpp"
#include "math/util.hpp"

#include <algorithm>
#include <limits>

using namespace raytrace;

bool raytrace::ray_aabb_intersection(const aabb &box, const ray &r,
				     /* out */ float &t0, /* out */ float &t1) {
  float t_near, t_far, tmp;
  t0 = r.min_t;
  t1 = r.max_t;

  for (int i = 0; i < 3; i++) {
    float inv_dir = 1.0f / r.d[i];
    t_near = (box.pmin[i] - r.o[i]) * inv_dir;
    t_far = (box.pmax[i] - r.o[i]) * inv_dir;

    if (t_near > t_far) {
      //swap near and far values
      tmp = t_near;
      t_near = t_far;
      t_far = tmp;
    }

    t0 = (t_near > t0) ? t_near : t0;
    t1 = (t_far < t1) ? t_far: t1;
    if (t0 > t1) return false;
  }

  return true;
}

float raytrace::aabb::volume() const {
  return (pmax.x - pmin.x)*(pmax.y - pmin.y)*(pmax.z - pmin.z);
}

float raytrace::aabb::surfacearea() const {
  float3 dp = pmax - pmin;
  float area_xy = dp.x * dp.y;
  float area_yz = dp.y * dp.z;
  float area_xz = dp.x * dp.z;

  return 2.0f * (area_xy + area_yz + area_xz);
}

float3 raytrace::aabb::center() const{
  return 0.5f * (pmin + pmax);
}

aabb raytrace::aabb::merge(const aabb &rhs) const {
  aabb result;
  result.pmin = float3{std::min(pmin.x, rhs.pmin.x), std::min(pmin.y, rhs.pmin.y), std::min(pmin.z, rhs.pmin.z)};
  result.pmax = float3{std::max(pmax.x, rhs.pmax.x), std::max(pmax.y, rhs.pmax.y), std::max(pmax.z, rhs.pmax.z)};
  return result;
}

aabb raytrace::aabb::empty_box() {
  float min_val = std::numeric_limits<float>::max();
  float max_val = -std::numeric_limits<float>::max();
  return {float3{min_val, min_val, min_val}, float3{max_val, max_val, max_val}};
}
