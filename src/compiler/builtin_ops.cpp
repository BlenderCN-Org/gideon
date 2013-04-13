#include "compiler/gd_std.hpp"
#include "math/vector.hpp"

#include <cstring>

#include <iostream>

using namespace raytrace;
using namespace gideon::rl;
using namespace std;

extern "C" void rtl_add_v3v3(float3 *result, float3 *lhs, float3 *rhs) {
  *result = *lhs + *rhs;
}

extern "C" void rtl_add_v4v4(float4 *result, float4 *lhs, float4 *rhs) {
  *result = *lhs + *rhs;
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

extern "C" void gd_builtin_light_iterator_begin(scene_data *s,
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
