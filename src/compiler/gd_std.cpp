#include "compiler/gd_std.hpp"
#include "scene/scene.hpp"
#include "geometry/triangle.hpp"

#include <random>
#include <functional>

using namespace gideon::rl;
using namespace raytrace;
using namespace std;

scene_data::scene_data(scene *s, bvh *accel) :
  s(s), accel(accel),
  rng(bind(uniform_real_distribution<float>(0.0f, 1.0f),
	   mt19937()))
{

}

/* Defines the C function versions of built-in Gideon functions. */

extern "C" bool gde_trace(ray *r, intersection *i,
			  int *aabb_count, int *prim_count, scene_data *s) {
  unsigned int aabb_checked, prim_checked;
  bool hit = s->accel->trace(*r, *i, aabb_checked, prim_checked);
  *aabb_count = static_cast<int>(aabb_checked);
  *prim_count = static_cast<int>(prim_checked);

  return hit;
}

extern "C" void gde_camera_shoot_ray(int x, int y, scene_data *sdata, ray *r) {
  *r = camera_shoot_ray(sdata->s->main_camera, x, y);
}

//Intersection Functions

extern "C" float gde_isect_dist(intersection *i) { return i->t; }

extern "C" void gde_isect_normal(intersection *i, scene_data *sdata, float3 *N) {
  scene *s = sdata->s;
  primitive &prim = s->primitives[i->prim_idx];
  int3 &tri = s->triangle_verts[prim.data_id];
  vector<float3> &verts = s->vertices;

  *N = compute_triangle_normal(verts[tri.x], verts[tri.y], verts[tri.z]);
}

extern "C" void gde_isect_smooth_normal(intersection *i, scene_data *sdata, float3 *N) {
  scene *s = sdata->s;
  primitive &prim = s->primitives[i->prim_idx];
  int3 &tri = s->triangle_verts[prim.data_id];
  vector<float3> &vnorms = s->vertex_normals;

  float inv = 1.0f - i->u - i->v;
  *N = normalize(inv*vnorms[tri.x] + i->u*vnorms[tri.y] + i->v*vnorms[tri.z]);
}

extern "C" void gde_ray_point_on_ray(ray *r, float t, float3 *P) {
  *P = r->point_on_ray(t);
}

extern "C" void gde_ray_origin(ray *r, float3 *O) { *O = r->o; }
extern "C" float gde_ray_max_dist(ray *r) { return r->max_t; }

//Vector Ops
extern "C" float gde_dot_v3(float3 *a, float3 *b) { return dot(*a, *b); }
extern "C" void gde_normalize_v3(float3 *in, float3 *out) { *out = normalize(*in); }
extern "C" float gde_length_v3(float3 *v) { return length(*v); }

//Misc Math
extern "C" float gde_exp_f(float x) { return expf(x); }

extern "C" float gde_random(void *s) { 
  scene_data *scn = reinterpret_cast<scene_data*>(s);
  return scn->rng();
}

//Shade-Tree Evaluation

extern "C" void gde_dfunc_eval(void *dfunc,
			       float3 *N,
			       float3 *P_in, float3 *w_in,
			       float3 *P_out, float3 *w_out,
			       /* out */ float4 *out) {
  shade_tree::node_ptr &node = *reinterpret_cast<shade_tree::node_ptr*>(dfunc);
  shade_tree::evaluate(node, N, P_in, w_in, P_out, w_out, out);
}

//Scene Access

extern "C" int gde_scene_num_lights(scene_data *sdata) {
  return static_cast<int>(sdata->s->lights.size());
}

extern "C" void gde_scene_get_light(scene_data *sdata, int id, light **light_id) {
  *light_id = &sdata->s->lights[id];
}

//Lights

extern "C" void gde_light_sample_position(light *lt, float3 *P, float rand_u, float rand_v,
					  float4 *P_out) {
  *P_out = lt->sample_position(*P, rand_u, rand_v);
}

extern "C" void gde_light_eval_radiance(light *lt, float3 *P, float3 *I, /* out */ float3 *R) {
  *R = lt->eval_radiance(*P, *I);
}

//Render Output

extern "C" void gde_write_pixel(int x, int y, int width, int height, float4 *color, void *out) {
  typedef float buffer_elem_type[4];
  float (*buffer)[4] = reinterpret_cast<buffer_elem_type*>(out);
  int idx = x + width*y;
  float *pix = buffer[idx];
  pix[0] = color->x;
  pix[1] = color->y;
  pix[2] = color->z;
  pix[3] = color->w;
}
