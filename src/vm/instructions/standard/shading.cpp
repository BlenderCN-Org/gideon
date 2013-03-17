#include "vm/vm.hpp"
#include "vm/instruction.hpp"

#include "shading/shadetree.hpp"
#include "scene/scene.hpp"

#include <stdexcept>

using namespace std;
using namespace raytrace;

/* Shading Instructions */

int light_eval_radiance_exec(vm *svm, const int *args) {
  light &l = svm->s->lights[svm->get<int>(args[1])];
  svm->get<float3>(args[0]) = l.eval_radiance(svm->get<float3>(args[2]),
					      svm->get<float3>(args[3]));
  
  return svm->pc() + 1;
}

/* Distribution Instructions */

int distribution_create_exec(vm *svm, const int *args) {
  int dist_id = svm->get<int>(args[1]);
  svm->get<shader_ref>(args[0]) = make_shared<shader_node>(make_shared<distribution_instance>(svm->s, dist_id));
  return svm->pc() + 1;
}

int distribution_destroy_exec(vm *svm, const int *args) {
  svm->get<shader_ref>(args[0]).reset();
  return svm->pc() + 1;
}

int distribution_get_params_exec(vm *svm, const int *args) {
  shader_node &node = *svm->get<shader_ref>(args[1]);
  if (node.type != shader_node::DISTRIBUTION) throw runtime_error("Cannot get parameters for this type of node.");
  distribution_instance &df = *node.func;
  svm->get<void*>(args[0]) = &df.params;

  return svm->pc() + 1;
}

int distribution_eval_exec(vm *svm, const int *args) {
  shader_node &node = *svm->get<shader_ref>(args[1]);
  if (node.type != shader_node::DISTRIBUTION) throw runtime_error("Cannot get parameters for this type of node.");
  distribution_instance &instance = *node.func;
  distribution_function &df = svm->s->distributions[instance.id];

  float3 &N = svm->get<float3>(args[2]);
  
  float3 &P_in = svm->get<float3>(args[3]);
  float3 &P_out = svm->get<float3>(args[4]);

  float3 &w_in = svm->get<float3>(args[5]);
  float3 &w_out = svm->get<float3>(args[6]);

  svm->get<float4>(args[0]) = df.evaluate(svm, &instance.params, N,
					  P_in, P_out,
					  w_in, w_out);
  return svm->pc() + 1;
};

/* Shader Building Instructions */

int shader_add_exec(vm *svm, const int *args) {
  shader_ref &lhs = svm->get<shader_ref>(args[1]);
  shader_ref &rhs = svm->get<shader_ref>(args[2]);
  svm->get<shader_ref>(args[0]) = make_shared<shader_node>(shader_node::SUM, lhs, rhs);
  
  return svm->pc() + 1;
}

int shader_mul_exec(vm *svm, const int *args) {
  shader_ref &lhs = svm->get<shader_ref>(args[1]);
  shader_ref &rhs = svm->get<shader_ref>(args[2]);
  svm->get<shader_ref>(args[0]) = make_shared<shader_node>(shader_node::PRODUCT, lhs, rhs);
  
  return svm->pc() + 1;
}

int shader_scale_f3_exec(vm *svm, const int *args) {
  float3 &k = svm->get<float3>(args[1]);
  shader_ref &op = svm->get<shader_ref>(args[2]);
  svm->get<shader_ref>(args[0]) = make_shared<shader_node>(float4{k.x, k.y, k.z, 1.0f}, op);

  return svm->pc() + 1;
}

int shader_scale_f4_exec(vm *svm, const int *args) {
  float4 &k = svm->get<float4>(args[1]);
  shader_ref &op = svm->get<shader_ref>(args[2]);
  svm->get<shader_ref>(args[0]) = make_shared<shader_node>(k, op);

  return svm->pc() + 1;
}

int shader_eval_exec(vm *svm, const int *args) {
  shader_node &node = *svm->get<shader_ref>(args[1]);
  
  float3 &N = svm->get<float3>(args[2]);
  
  float3 &P_in = svm->get<float3>(args[3]);
  float3 &P_out = svm->get<float3>(args[4]);

  float3 &w_in = svm->get<float3>(args[5]);
  float3 &w_out = svm->get<float3>(args[6]);
  
  svm->get<float4>(args[0]) = evaluate_shade_formula(svm, node,
						     N,
						     P_in, P_out,
						     w_in, w_out);
  return svm->pc() + 1;
};

/* Instruction Creation */

void create_shading_instructions(instruction_set &instr) {
  instr.add_instruction({"light_eval_radiance", 4, light_eval_radiance_exec});
  
  instr.add_instruction({"distribution_create", 2, distribution_create_exec});
  instr.add_instruction({"distribution_destroy", 1, distribution_destroy_exec});
  instr.add_instruction({"distribution_get_params", 2, distribution_get_params_exec});
  
  instr.add_instruction({"shader_add", 3, shader_add_exec});
  instr.add_instruction({"shader_mul", 3, shader_mul_exec});
  instr.add_instruction({"shader_scale_f3", 3, shader_scale_f3_exec});
  instr.add_instruction({"shader_scale_f4", 3, shader_scale_f4_exec});

  instr.add_instruction({"shader_eval", 7, shader_eval_exec});

  instr.add_instruction({"distribution_eval", 7, distribution_eval_exec});
}
