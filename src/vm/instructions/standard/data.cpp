#include "math/vector.hpp"
#include "vm/vm.hpp"
#include "vm/instruction.hpp"

#include <iostream>
#include <sstream>

using namespace std;
using namespace raytrace;

/* Instructions for moving/converting/accessing data */

/* Data Copy Instructions */

template<typename T>
int mov_R_C_exec(vm *svm, const int *args) {
  svm->get<T>(args[0]) = svm->constant<T>(args[1]);
  return svm->pc() + 1;
}

template<typename T>
int mov_R_R_exec(vm *svm, const int *args) {
  svm->get<T>(args[0]) = svm->get<T>(args[1]);
  return svm->pc() + 1;
}

/* Parameter Access */

template<typename T>
int movi_R_C_exec(vm *svm, const int *args) {
  svm->get<T>(args[0]) = svm->input_param<T>(svm->constant<int>(args[1]));
  return svm->pc() + 1;
}

template<typename T>
int movi_R_R_exec(vm *svm, const int *args) {
  svm->get<T>(args[0]) = svm->input_param<T>(svm->get<int>(args[1]));
  return svm->pc() + 1;
}

template<typename T>
int movo_R_C_exec(vm *svm, const int *args) {
  svm->output_param<T>(svm->constant<int>(args[1])) = svm->get<T>(args[0]);
  return svm->pc() + 1;
}

template<typename T>
int movo_R_R_exec(vm *svm, const int *args) {
  svm->output_param<T>(svm->get<int>(args[1])) = svm->get<T>(args[0]);
  return svm->pc() + 1;
}

template<typename T>
int movp_R_C_exec(vm *svm, const int *args) {
  parameter_list *params = reinterpret_cast<parameter_list*>(svm->get<void*>(args[0]));
  params->get<T>(svm->constant<int>(args[2])) = svm->get<T>(args[1]);

  return svm->pc() + 1;
}

template<typename T>
int movp_R_R_exec(vm *svm, const int *args) {
  parameter_list *params = reinterpret_cast<parameter_list*>(svm->get<void*>(args[0]));
  params->get<T>(svm->get<int>(args[2])) = svm->get<T>(args[1]);

  return svm->pc() + 1;
}

/* Conversion Instructions */

template<typename A, typename B>
int conv_exec(vm *svm, const int *args) {
  svm->get<A>(args[0]) = svm->get<B>(args[1]);
  return svm->pc() + 1;
}

template<typename T>
int merge2_exec(vm *svm, const int *args) {
  svm->get< gen_vec2<T> >(args[0]) = {svm->get<T>(args[1]), svm->get<T>(args[2])};
  return svm->pc() + 1;
}

template<typename T>
int merge3_exec(vm *svm, const int *args) {
  svm->get< gen_vec3<T> >(args[0]) = {svm->get<T>(args[1]), svm->get<T>(args[2]), svm->get<T>(args[3])};
  return svm->pc() + 1;
}

template<typename T>
int merge4_exec(vm *svm, const int *args) {
  svm->get< gen_vec4<T> >(args[0]) = {svm->get<T>(args[1]), svm->get<T>(args[2]), svm->get<T>(args[3]), svm->get<T>(args[4])};
  return svm->pc() + 1;
}

template<typename Scalar, typename Vector>
int vector_elem_C_exec(vm *svm, const int *args) {
  svm->get<Scalar>(args[0]) = svm->constant<Vector>(args[1])[args[2]];
  return svm->pc() + 1;
}

template<typename Scalar, typename Vector>
int vector_elem_R_exec(vm *svm, const int *args) {
  svm->get<Scalar>(args[0]) = svm->get<Vector>(args[1])[args[2]];
  return svm->pc() + 1;
}

/* Instruction Creation */

template<typename T>
void create_mov_instructions(instruction_set &instructions, const string &typecode) {
  stringstream ss_rc, ss_rr;
  ss_rc << "mov_" << typecode << "_" << typecode << "C";
  ss_rr << "mov_" << typecode << "_" << typecode;

  instructions.add_instruction({ss_rc.str(), 2, mov_R_C_exec<T>});
  instructions.add_instruction({ss_rr.str(), 2, mov_R_R_exec<T>});
}

template<typename T>
void create_merge_instructions(instruction_set &instructions, const string &typecode) {
  stringstream ss2, ss3, ss4;
  ss2 << "merge2_" << typecode;
  ss3 << "merge3_" << typecode;
  ss4 << "merge4_" << typecode;

  instructions.add_instruction({ss2.str(), 3, merge2_exec<T>});
  instructions.add_instruction({ss3.str(), 4, merge3_exec<T>});
  instructions.add_instruction({ss4.str(), 5, merge4_exec<T>});
}

template<typename T>
void create_access_instructions(instruction_set &instructions, const string &typecode) {
  stringstream ss_r2, ss_r3, ss_r4, ss_c2, ss_c3, ss_c4;
  ss_r2 << "vector_elem_" << typecode << "2";
  ss_c2 << "vector_elem_" << typecode << "2C";

  ss_r3 << "vector_elem_" << typecode << "3";
  ss_c3 << "vector_elem_" << typecode << "3C";

  ss_r4 << "vector_elem_" << typecode << "4";
  ss_c4 << "vector_elem_" << typecode << "4C";

  instructions.add_instruction({ss_r2.str(), 3, vector_elem_R_exec< T, gen_vec2<T> >});
  instructions.add_instruction({ss_c2.str(), 3, vector_elem_C_exec< T, gen_vec2<T> >});

  instructions.add_instruction({ss_r3.str(), 3, vector_elem_R_exec< T, gen_vec3<T> >});
  instructions.add_instruction({ss_c3.str(), 3, vector_elem_C_exec< T, gen_vec3<T> >});

  instructions.add_instruction({ss_r4.str(), 3, vector_elem_R_exec< T, gen_vec4<T> >});
  instructions.add_instruction({ss_c4.str(), 3, vector_elem_C_exec< T, gen_vec4<T> >});
}

template<typename T>
void create_io_instructions(instruction_set &instr, const string &typecode) {
  stringstream ss_irr, ss_irc, ss_orr, ss_orc, ss_prc, ss_prr;
  ss_irc << "movi_" << typecode << "_iC";
  ss_irr << "movi_" << typecode << "_i";

  ss_orc << "movo_" << typecode << "_iC";
  ss_orr << "movo_" << typecode << "_i";

  ss_prc << "movp_" << typecode << "_iC";
  ss_prr << "movp_" << typecode << "_i";
  
  instr.add_instruction({ss_irr.str(), 2, movi_R_R_exec<T>});
  instr.add_instruction({ss_irc.str(), 2, movi_R_C_exec<T>});

  instr.add_instruction({ss_orr.str(), 2, movo_R_R_exec<T>});
  instr.add_instruction({ss_orc.str(), 2, movo_R_C_exec<T>});

  instr.add_instruction({ss_prr.str(), 3, movp_R_R_exec<T>});
  instr.add_instruction({ss_prc.str(), 3, movp_R_C_exec<T>});
}

void create_data_instructions(instruction_set &instr) {
  create_mov_instructions<int>(instr, "i");
  create_mov_instructions<float>(instr, "f");
  create_mov_instructions<float2>(instr, "f2");
  create_mov_instructions<float3>(instr, "f3");
  create_mov_instructions<float4>(instr, "f4");

  create_mov_instructions<string>(instr, "s");
  create_mov_instructions<ray>(instr, "ray");

  instr.add_instruction({"conv_i_to_f", 2, conv_exec<float, int>});
  instr.add_instruction({"conv_f_to_i", 2, conv_exec<int, float>});

  create_merge_instructions<int>(instr, "i");
  create_merge_instructions<float>(instr, "f");
  
  create_access_instructions<int>(instr, "i");
  create_access_instructions<float>(instr, "f");

  create_io_instructions<int>(instr, "i");
  create_io_instructions<float>(instr, "f");
  create_io_instructions<float2>(instr, "f2");
  create_io_instructions<float3>(instr, "f3");
  create_io_instructions<float4>(instr, "f4");
}
