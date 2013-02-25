#include "math/vector.hpp"
#include "vm/vm.hpp"
#include "vm/instruction.hpp"

#include "scene/scene.hpp"
#include "scene/attribute_reader.hpp"

#include <iostream>
#include <sstream>

using namespace std;
using namespace raytrace;

/* Primitive/Object Attribute Access */

template<typename T>
int prim_attribute_exec(vm *svm, const int *args) {
  primitive *p = svm->get<primitive*>(args[2]);
  string &name = svm->constant<string>(args[3]);
  
  bool found = primitive_get_attribute<T>(*p, *svm->s,
					  name, {svm->get<float>(args[4]), svm->get<float>(args[5]), 0.0f, 0.0f},
					  svm->get<T>(args[1]));
  
  svm->get<int>(args[0]) = (found ? 1 : 0);
  return svm->pc() + 1;
}

int prim_geometry_normal_exec(vm *svm, const int *args) {
  primitive *p = svm->get<primitive*>(args[1]);
  float u = svm->get<float>(args[2]);
  float v = svm->get<float>(args[3]);

  svm->get<float3>(args[0]) = primitive_geometry_normal(*p, *svm->s);
  return svm->pc() + 1;
}

int light_count_exec(vm *svm, const int *args) {
  svm->get<int>(args[0]) = static_cast<int>(svm->s->lights.size());
  return svm->pc() + 1;
}

int light_sample_position_exec(vm *svm, const int *args) {
  int idx = svm->get<int>(args[1]);
  light &l = svm->s->lights[idx];

  svm->get<float4>(args[0]) = l.sample_position(svm->get<float3>(args[2]),
						svm->get<float>(args[3]),
						svm->get<float>(args[4]));
  
  return svm->pc() + 1;
}

int random_f_exec(vm *svm, const int *args) {
  svm->get<float>(args[0]) = svm->random();
  return svm->pc() + 1;
}

/* Instruction Creation */

void create_attribute_instructions(instruction_set &instr) {
  instr.add_instruction({"prim_attribute_i", 6, prim_attribute_exec<int>});

  instr.add_instruction({"prim_attribute_f", 6, prim_attribute_exec<float>});
  instr.add_instruction({"prim_attribute_f2", 6, prim_attribute_exec<float2>});
  instr.add_instruction({"prim_attribute_f3", 6, prim_attribute_exec<float3>});

  instr.add_instruction({"prim_geometry_normal", 4, prim_geometry_normal_exec});

  instr.add_instruction({"light_count", 1, light_count_exec});
  instr.add_instruction({"light_sample_position", 5, light_sample_position_exec});

  instr.add_instruction({"random_f", 1, random_f_exec});
}
