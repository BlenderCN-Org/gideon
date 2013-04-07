#include "math/vector.hpp"
#include <cstring>

using namespace raytrace;
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
