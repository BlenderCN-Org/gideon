#include "math/vector.hpp"

using namespace raytrace;
using namespace std;

extern "C" void rtl_add_v4v4(float4 *result, float4 *lhs, float4 *rhs) {
  *result = *lhs + *rhs;
}
