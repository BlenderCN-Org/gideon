#include "vm/vm.hpp"
#include "vm/instruction.hpp"

#include "scene/scene.hpp"

#include <sstream>

using namespace std;
using namespace raytrace;

/* Geometry Instructions */

int point_on_ray_exec(vm *svm, const int *args) {
  svm->get<float3>(args[0]) = svm->get<ray>(args[1]).point_on_ray(svm->get<float>(args[2]));
  return svm->pc() + 1;
}

int ray_dir_exec(vm *svm, const int *args) {
  svm->get<float3>(args[0]) = svm->get<ray>(args[1]).d;
  return svm->pc() + 1;
}

int isect_depth_exec(vm *svm, const int *args) {
  svm->get<float>(args[0]) = svm->get<intersection>(args[1]).t;
  return svm->pc() + 1;
}

int isect_primitive_exec(vm *svm, const int *args) {
  intersection &isect = svm->get<intersection>(args[1]);
  svm->get<primitive*>(args[0]) = &svm->s->primitives[isect.prim_idx];
  return svm->pc() + 1;
}

int isect_u_v_exec(vm *svm, const int *args) {
  intersection &isect = svm->get<intersection>(args[0]);
  svm->get<float>(args[1]) = isect.u;
  svm->get<float>(args[2]) = isect.v;
  return svm->pc() + 1;
}

/* Instruction Creation */

void create_geometry_instructions(instruction_set &instr) {
  instr.add_instruction({"point_on_ray", 3, point_on_ray_exec});

  instr.add_instruction({"ray_dir", 2, ray_dir_exec});

  instr.add_instruction({"isect_depth", 2, isect_depth_exec});
  instr.add_instruction({"isect_primitive", 2, isect_primitive_exec});
  instr.add_instruction({"isect_u_v", 3, isect_u_v_exec});
}
