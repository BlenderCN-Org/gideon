#include "compiler/gd_std.hpp"

using namespace gideon::rl;
using namespace raytrace;
using namespace std;

/* Defines the C function versions of built-in Gideon functions. */

extern "C" bool gde_trace(ray *r, intersection *i, scene_data *s) {
  unsigned int aabb_checked, prim_checked;
  if (s->accel->trace(*r, *i, aabb_checked, prim_checked)) return true;
  return false;
}

extern "C" void gde_camera_shoot_ray(int x, int y, scene_data *sdata, ray *r) {
  *r = camera_shoot_ray(sdata->s->main_camera, x, y);
}

extern "C" float gde_isect_dist(intersection *i) { return i->t; }

extern "C" float gde_ray_max_dist(ray *r) { return r->max_t; }

//Vector Ops
extern "C" float gde_dot_v3(float3 *a, float3 *b) { return dot(*a, *b); }
extern "C" void gde_normalize_v3(float3 *in, float3 *out) { *out = normalize(*in); }
extern "C" float gde_length_v3(float3 *v) { return length(*v); }

//Shade-Tree Evaluation

extern "C" void gde_dfunc_eval(void *dfunc,
			       float3 *N,
			       float3 *P_in, float3 *w_in,
			       float3 *P_out, float3 *w_out,
			       /* out */ float4 *out) {
  shade_tree::node_ptr &node = *reinterpret_cast<shade_tree::node_ptr*>(dfunc);
  shade_tree::evaluate(node, N, P_in, w_in, P_out, w_out, out);
}
