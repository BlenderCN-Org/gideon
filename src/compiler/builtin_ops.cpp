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
#include "shading/distribution.hpp"
#include "math/vector.hpp"

#include "engine/context.hpp"

#include <cstring>

#include <iostream>

using namespace gideon;
using namespace raytrace;
using namespace gideon::rl;
using namespace std;

extern "C" void gd_builtin_add_v2_v2(float2 *result, float2 *lhs, float2 *rhs) {
  *result = *lhs + *rhs;
}

extern "C" void gd_builtin_mul_v2_v2(float2 *result, float2 *lhs, float2 *rhs) {
  *result = *lhs * *rhs;
}

extern "C" void gd_builtin_div_v2_v2(float2 *result, float2 *lhs, float2 *rhs) {
  *result = *lhs / *rhs;
}

extern "C" void gd_builtin_sub_v2_v2(float2 *result, float2 *lhs, float2 *rhs) {
  *result = *lhs - *rhs;
}


extern "C" void gd_builtin_add_v3_v3(float3 *result, float3 *lhs, float3 *rhs) {
  *result = *lhs + *rhs;
}

extern "C" void gd_builtin_mul_v3_v3(float3 *result, float3 *lhs, float3 *rhs) {
  *result = *lhs * *rhs;
}

extern "C" void gd_builtin_div_v3_v3(float3 *result, float3 *lhs, float3 *rhs) {
  *result = *lhs / *rhs;
}

extern "C" void gd_builtin_sub_v3_v3(float3 *result, float3 *lhs, float3 *rhs) {
  *result = *lhs - *rhs;
}


extern "C" void gd_builtin_add_v4_v4(float4 *result, float4 *lhs, float4 *rhs) {
  *result = *lhs + *rhs;
}

extern "C" void gd_builtin_mul_v4_v4(float4 *result, float4 *lhs, float4 *rhs) {
  *result = *lhs * *rhs;
}

extern "C" void gd_builtin_div_v4_v4(float4 *result, float4 *lhs, float4 *rhs) {
  *result = *lhs / *rhs;
}

extern "C" void gd_builtin_sub_v4_v4(float4 *result, float4 *lhs, float4 *rhs) {
  *result = *lhs - *rhs;
}


extern "C" void gd_builtin_scale_v2(float2 *result, float *k, float2 *rhs) {
  *result = *k * (*rhs);
}

extern "C" void gd_builtin_scale_v3(float3 *result, float *k, float3 *rhs) {
  *result = *k * (*rhs);
}

extern "C" void gd_builtin_scale_v4(float4 *result, float *k, float4 *rhs) {
  *result = *k * (*rhs);
}

extern "C" void gd_builtin_inv_scale_v2(float2 *result, float *k, float2 *rhs) {
  *result = (*rhs) / *k;
}

extern "C" void gd_builtin_inv_scale_v3(float3 *result, float *k, float3 *rhs) {
  *result = (*rhs) / *k;
}

extern "C" void gd_builtin_inv_scale_v4(float4 *result, float *k, float4 *rhs) {
  *result = (*rhs) / *k;
}

/* String Functions */

extern "C" void gd_builtin_copy_string(bool const0, char *s0,
				       /* out */ bool *const1, /* out */ char **s1) {
  if (const0) {
    *const1 = true;
    *s1 = s0;
  }
  else {
    *const1 = false;
    size_t len = strlen(s0) + 1;
    *s1 = new char[len];
    memcpy(*s1, s0, len);
  }
}

extern "C" void gd_builtin_destroy_string(bool is_const, char *str) {
  if (!is_const) delete[] str;
}

extern "C" char *gd_builtin_concat_string(char *s0, char *s1) {  
  size_t len0 = strlen(s0);
  size_t len1 = strlen(s1);
  
  char *s_out = new char[len0 + len1 + 1];
  memcpy(s_out, s0, len0);
  memcpy(s_out + len0, s1, len1);
  s_out[len0 + len1] = 0;
  
  return s_out;
}

/* Scene Access Functions */

extern "C" void gd_builtin_light_iterator_begin(render_context::scene_data *s,
						/* out */ void **s_ptr, /* out */ int *light_idx) {
  *s_ptr = s->s;
  *light_idx = 0;
}

extern "C" bool gd_builtin_light_iterator_has_next(void *s_ptr, int light_id) {
  scene *s = reinterpret_cast<scene*>(s_ptr);
  return (light_id < s->lights.size());
}

extern "C" void gd_builtin_light_iterator_next(void *s_ptr, /* inout */ int *light_id,
					       /* out */ light **lt) {
  scene *s = reinterpret_cast<scene*>(s_ptr);
  *lt = &s->lights[(*light_id)++];
}

/* Distributions */

extern "C" void *gd_builtin_alloc_dfunc(void *,
					int param_size,
					uint64_t flags,
					shade_tree::leaf::eval_func_type eval,
					shade_tree::leaf::sample_func_type sample,
					shade_tree::leaf::pdf_func_type pdf,
					shade_tree::leaf::emission_func_type emit,
					shade_tree::leaf::dtor_func_type dtor,
					/* out */ void *out) {
  char *params = new char[param_size];
  new (out) shade_tree::node_ptr(shade_tree::leaf_ptr(new shade_tree::leaf(params, flags, eval, sample, pdf, emit, dtor)));
  return params;
}

extern "C" void gd_builtin_copy_dfunc(void *in, /* out */ void *out) {
  shade_tree::node_ptr *node_in = reinterpret_cast<shade_tree::node_ptr*>(in);
  new (out) shade_tree::node_ptr(*node_in);
}

extern "C" void gd_builtin_destroy_dfunc(void *out) {
  typedef shade_tree::node_ptr node_type;
  
  node_type *node = reinterpret_cast<shade_tree::node_ptr*>(out);
  node->~node_type();
}

extern "C" void gd_builtin_dfunc_add(void *lhs, void *rhs, /* out */ void *out) {
  shade_tree::node_ptr *left = reinterpret_cast<shade_tree::node_ptr*>(lhs);
  shade_tree::node_ptr *right = reinterpret_cast<shade_tree::node_ptr*>(rhs);
  
  new (out) shade_tree::node_ptr(shade_tree::sum_ptr(new shade_tree::sum(*left, *right)));
}

extern "C" void gd_builtin_dfunc_scale(float4 *k, void *d, /* out */ void *out) {
  shade_tree::node_ptr *node = reinterpret_cast<shade_tree::node_ptr*>(d);
  new (out) shade_tree::node_ptr(shade_tree::scale_ptr(new shade_tree::scale(*k, *node)));
}

typedef struct { bool is_const; char *data; } gd_error_string_type;

extern "C" void gd_builtin_error(void *str) {  
  gd_error_string_type *ptr = (gd_error_string_type*)(str);
  throw *ptr;
}

extern "C" void gd_builtin_handle_error(void *str) {
  gd_error_string_type *ptr = (gd_error_string_type*)(str);
  cerr << "Gideon Error: " << ptr->data << endl;
  gd_builtin_destroy_string(ptr->is_const, ptr->data);
}
