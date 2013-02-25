#include "vm/vm.hpp"
#include "vm/instruction.hpp"

#include <sstream>

using namespace std;
using namespace raytrace;

/* Instructions for Function Calls / Branching */

int jump_exec(vm *svm, const int *args) {
  return args[0];
}

int jump_R_exec(vm *svm, const int *args) {
  return svm->get<int>(args[0]);
}

int jump_z_exec(vm *svm, const int *args) {
  if (svm->get<int>(args[0]) == 0) return args[1];
  return svm->pc() + 1;
}

int jump_z_R_exec(vm *svm, const int *args) {
  if (svm->get<int>(args[0]) == 0) return svm->get<int>(args[1]);
  return svm->pc() + 1;
}

int jump_nz_exec(vm *svm, const int *args) {
  if (svm->get<int>(args[0]) != 0) return args[1];
  return svm->pc() + 1;
}

int jump_nz_R_exec(vm *svm, const int *args) {
  if (svm->get<int>(args[0]) != 0) return svm->get<int>(args[1]);
  return svm->pc() + 1;
}

int jump_g_exec(vm *svm, const int *args) {
  if (svm->get<int>(args[0]) > svm->get<int>(args[1])) return args[2];
  return svm->pc() + 1;
}

int jump_l_exec(vm *svm, const int *args) {
  if (svm->get<int>(args[0]) < svm->get<int>(args[1])) return args[2];
  return svm->pc() + 1;
}

int save_pc_offset_exec(vm *svm, const int *args) {
  svm->get<int>(args[0]) = svm->pc() + args[1];
  return svm->pc() + 1;
}

int stack_push_exec(vm *svm, const int *args) {
  svm->push_stack_frame();
  return svm->pc() + 1;
}

int stack_pop_exec(vm *svm, const int *args) {
  svm->pop_stack_frame();
  return svm->pc() + 1;
}

/* Instruction Creation */

void create_control_instructions(instruction_set &instr) {
  instr.add_instruction({"jump", 1, jump_exec});
  instr.add_instruction({"jump_R", 1, jump_R_exec});
  
  instr.add_instruction({"jump_z", 2, jump_z_exec});
  instr.add_instruction({"jump_z_R", 2, jump_z_R_exec});
  
  instr.add_instruction({"jump_nz", 2, jump_nz_exec});
  instr.add_instruction({"jump_nz_R", 2, jump_nz_R_exec});

  instr.add_instruction({"jump_g", 3, jump_g_exec});
  instr.add_instruction({"jump_l", 3, jump_l_exec});

  instr.add_instruction({"save_pc_offset", 2, save_pc_offset_exec});
  instr.add_instruction({"stack_push", 0, stack_push_exec});
  instr.add_instruction({"stack_pop", 0, stack_pop_exec});
}
