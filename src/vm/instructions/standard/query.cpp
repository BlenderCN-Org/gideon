#include "math/vector.hpp"
#include "vm/vm.hpp"
#include "vm/instruction.hpp"

#include "scene/scene.hpp"
#include "scene/bvh.hpp"

#include <iostream>
#include <sstream>

using namespace std;
using namespace raytrace;

/* Scene Query Instructions */

int gen_camera_ray_exec(vm *svm, const int *args) {
  svm->get<ray>(args[0]) = camera_shoot_ray(svm->s->main_camera,
					    svm->get<float>(args[1]),
					    svm->get<float>(args[2]));
  return svm->pc() + 1;
};

int build_ray_exec(vm *svm, const int *args) {
  svm->get<ray>(args[0]) = {svm->get<float3>(args[1]),
			    svm->get<float3>(args[2]),
			    svm->get<float>(args[3]),
			    svm->get<float>(args[4])};
  return svm->pc() + 1;
}

int trace_exec(vm *svm, const int *args) {
  unsigned int aabb_checked, prim_checked;
  svm->get<int>(args[0]) = svm->accel->trace(svm->get<ray>(args[1]), svm->get<intersection>(args[2]),
					     aabb_checked, prim_checked);
  
  svm->get<int>(args[3]) = static_cast<int>(aabb_checked);
  svm->get<int>(args[4]) = static_cast<int>(prim_checked);
  return svm->pc() + 1;
}

/* Instruction Creation */

void create_query_instructions(instruction_set &instr) {
  instr.add_instruction({"gen_camera_ray", 3, gen_camera_ray_exec});
  instr.add_instruction({"build_ray", 5, build_ray_exec});
  instr.add_instruction({"trace", 5, trace_exec});
}
