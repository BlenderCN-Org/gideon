/*

  Copyright 2013 Curtis Andrus

  This file is part of Gideon.

  Gideon is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  Gideon is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with Gideon.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "compiler/gd_std.hpp"
#include "scene/scene.hpp"
#include "scene/attribute_reader.hpp"

#include "geometry/triangle.hpp"
#include "math/sampling.hpp"

#include "engine/context.hpp"

#include <random>
#include <functional>

using namespace gideon;
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
			  int *aabb_count, int *prim_count, render_context::scene_data *s) {
  unsigned int aabb_checked, prim_checked;
  bool hit = s->accel->trace(*r, *i, aabb_checked, prim_checked);
  *aabb_count = static_cast<int>(aabb_checked);
  *prim_count = static_cast<int>(prim_checked);

  return hit;
}

extern "C" void gde_camera_shoot_ray(int x, int y, render_context::scene_data *sdata, ray *r) {
  *r = camera_shoot_ray(sdata->s->main_camera, x, y);
}

extern "C" void gde_camera_shoot_ray_f(float x, float y, render_context::scene_data *sdata, ray *r) {
  *r = camera_shoot_ray(sdata->s->main_camera, x, y);
}

//Primitive Functions

extern "C" void *gde_primitive_shader(render_context::scene_data *sdata, int prim_id) {
  scene *s = sdata->s;
  primitive &prim = s->primitives[prim_id];
  return prim.shader_id;
}

extern "C" void *gde_primitive_volume_shader(render_context::scene_data *sdata, int prim_id) {
  scene *s = sdata->s;
  primitive &prim = s->primitives[prim_id];
  return prim.volume_id;
}

extern "C" bool gde_primitive_has_surface(render_context::scene_data *sdata, int prim_id) {
  scene *s = sdata->s;
  primitive &prim = s->primitives[prim_id];
  return (prim.shader_id != NULL);
}

extern "C" bool gde_primitive_has_volume(render_context::scene_data *sdata, int prim_id) {
  scene *s = sdata->s;
  primitive &prim = s->primitives[prim_id];
  return (prim.volume_id != NULL);
}

typedef struct { bool is_const; char *data; } gd_string_type;

extern "C" bool gde_primitive_get_attribute_f(render_context::scene_data *sdata, int prim_id,
					      gd_string_type *attr_name, float4 *coords,
					      /* out */ float *result) {
  scene *s = sdata->s;
  primitive &prim = s->primitives[prim_id];

  return primitive_get_attribute<float>(prim, *s, attr_name->data, *coords, *result);
}

extern "C" bool gde_primitive_get_attribute_v2(render_context::scene_data *sdata, int prim_id,
					       gd_string_type *attr_name, float4 *coords,
					       /* out */ float2 *result) {
  scene *s = sdata->s;
  primitive &prim = s->primitives[prim_id];

  return primitive_get_attribute<float2>(prim, *s, attr_name->data, *coords, *result);
}

extern "C" bool gde_primitive_get_attribute_v3(render_context::scene_data *sdata, int prim_id,
					       gd_string_type *attr_name, float4 *coords,
					       /* out */ float3 *result) {
  scene *s = sdata->s;
  primitive &prim = s->primitives[prim_id];

  return primitive_get_attribute<float3>(prim, *s, attr_name->data, *coords, *result);
}



//Intersection Functions

extern "C" float gde_isect_dist(intersection *i) { return i->t; }

extern "C" void gde_isect_normal(intersection *i, render_context::scene_data *sdata, float3 *N) {
  scene *s = sdata->s;
  primitive &prim = s->primitives[i->prim_idx];
  int3 &tri = s->triangle_verts[prim.data_id];
  vector<float3> &verts = s->vertices;

  *N = compute_triangle_normal(verts[tri.x], verts[tri.y], verts[tri.z]);
}

extern "C" void gde_isect_smooth_normal(intersection *i, render_context::scene_data *sdata, float3 *N) {
  scene *s = sdata->s;
  primitive &prim = s->primitives[i->prim_idx];
  int3 &tri = s->triangle_verts[prim.data_id];
  vector<float3> &vnorms = s->vertex_normals;

  float inv = 1.0f - i->u - i->v;
  *N = normalize(inv*vnorms[tri.x] + i->u*vnorms[tri.y] + i->v*vnorms[tri.z]);
}

extern "C" int gde_isect_primitive_id(intersection *i) {
  return i->prim_idx;
}

/* Ray */

extern "C" void gde_ray_point_on_ray(ray *r, float t, float3 *P) {
  *P = r->point_on_ray(t);
}

extern "C" void gde_isect_uv(intersection *i, /* out */ float2 *uv) {
  *uv = float2{i->u, i->v};
}

extern "C" void gde_ray_origin(ray *r, float3 *O) { *O = r->o; }
extern "C" void gde_ray_direction(ray *r, float3 *D) { *D = r->d; }
extern "C" float gde_ray_max_dist(ray *r) { return r->max_t; }

//Vector Ops
extern "C" float gde_dot_v3(float3 *a, float3 *b) { return dot(*a, *b); }
extern "C" void gde_normalize_v3(float3 *in, float3 *out) { *out = normalize(*in); }
extern "C" float gde_length_v3(float3 *v) { return length(*v); }

//Misc Math
extern "C" float gde_exp_f(float x) { return expf(x); }
extern "C" float gde_pow_f(float x, float y) { return powf(x, y); }

extern "C" float gde_random(void *s) { 
  render_context::scene_data *scn = reinterpret_cast<render_context::scene_data*>(s);
  return scn->rng();
}

extern "C" void gde_cosine_sample_hemisphere(float3 *N,
					     float rand_u, float rand_v,
					     /* out */ float3 *rt) {
  *rt = cosine_sample_hemisphere(*N, rand_u, rand_v);
}

extern "C" void gde_make_orthonormals(float3 *N,
				      /* out */ float3 *T, /* out */ float3 *B) {
  make_orthonormals(*N, *T, *B);
}

extern "C" void gde_spherical_direction(float3 *N, float3 *T, float3 *B,
					float sin_theta, float cos_theta, float phi,
					/* out */ float3 *rt) {
  *rt = spherical_direction(*N, *T, *B, sin_theta, cos_theta, phi);
}

//Shade-Tree Evaluation

extern "C" void gde_dfunc_eval(void *dfunc,
			       shade_tree::shader_flags mask,
			       float3 *P_in, float3 *w_in,
			       float3 *P_out, float3 *w_out,
			       /* out */ float *pdf, /* out */ float4 *out) {
  shade_tree::node_ptr &node = *reinterpret_cast<shade_tree::node_ptr*>(dfunc);
  shade_tree::evaluate(node, mask, P_in, w_in, P_out, w_out, pdf, out);
}

extern "C" float gde_dfunc_pdf(void *dfunc,
			       shade_tree::shader_flags mask,
			       float3 *P_in, float3 *w_in,
			       float3 *P_out, float3 *w_out) {
  shade_tree::node_ptr &node = *reinterpret_cast<shade_tree::node_ptr*>(dfunc);
  return shade_tree::pdf(node, mask, P_in, w_in, P_out, w_out);
}

extern "C" float gde_dfunc_sample(void *dfunc,
				  shade_tree::shader_flags mask,
				  float3 *P_out, float3 *w_out,
				  float rand_D, float2 *rand_P, float2 *rand_w,
				  /* out */ float3 *P_in, /* out */ float3 *w_in) {
  shade_tree::node_ptr &node = *reinterpret_cast<shade_tree::node_ptr*>(dfunc);
  return shade_tree::sample(node, mask, P_out, w_out, rand_D, rand_P, rand_w, P_in, w_in);
}

extern "C" void gde_dfunc_emission(void *dfunc,
				   shade_tree::shader_flags mask,
				   float3 *P_out, float3 *w_out,
				   /* out */ float4 *Le) {
  shade_tree::node_ptr &node = *reinterpret_cast<shade_tree::node_ptr*>(dfunc);
  shade_tree::emission(node, mask, P_out, w_out, Le);
}

extern "C" shade_tree::shader_flags gde_dfunc_flags(void *dfunc) {
  shade_tree::node_ptr &node = *reinterpret_cast<shade_tree::node_ptr*>(dfunc);
  return shade_tree::get_flags(node);
}

extern "C" bool gde_shader_handle_is_valid(void *shader) {
  return (shader != nullptr);
}

//Scene Access

extern "C" int gde_scene_num_lights(render_context::scene_data *sdata) {
  return static_cast<int>(sdata->s->lights.size());
}

extern "C" void gde_scene_get_light(render_context::scene_data *sdata, int id, light **light_id) {
  *light_id = &sdata->s->lights[id];
}

//Lights

extern "C" void gde_light_sample_position(light *lt, float3 *P, float rand_u, float rand_v,
					  float *pdf, float4 *P_out) {
  *P_out = lt->sample_position(*P, rand_u, rand_v, *pdf);
}

extern "C" void gde_light_eval_radiance(light *lt, float3 *P, float3 *I, /* out */ float4 *R) {
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
