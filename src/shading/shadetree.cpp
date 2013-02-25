#include "shading/shadetree.hpp"

#include "scene/scene.hpp"
#include "vm/vm.hpp"

using namespace std;
using namespace raytrace;

float4 raytrace::evaluate_shade_formula(vm *svm, shader_node &n,
					const float3 &N,
					const float3 &P_in, const float3 &P_out,
					const float3 &w_in, const float3 &w_out) {
  if (n.type == shader_node::DISTRIBUTION) {
    distribution_instance &inst = *n.func;
    distribution_function &func = svm->s->distributions[inst.id];
    return func.evaluate(svm, &inst.params,
			 N, P_in, P_out, w_in, w_out);
  }
  else if (n.type == shader_node::SUM) {
    return (evaluate_shade_formula(svm, *n.lhs, N, P_in, P_out, w_in, w_out) +
	    evaluate_shade_formula(svm, *n.rhs, N, P_in, P_out, w_in, w_out));
  }
  else if (n.type == shader_node::PRODUCT) {
    return (evaluate_shade_formula(svm, *n.lhs, N, P_in, P_out, w_in, w_out) *
	    evaluate_shade_formula(svm, *n.rhs, N, P_in, P_out, w_in, w_out));
  }
  else if (n.type == shader_node::SCALE) {
    return (n.k * evaluate_shade_formula(svm, *n.rhs, N, P_in, P_out, w_in, w_out));
  }
}
