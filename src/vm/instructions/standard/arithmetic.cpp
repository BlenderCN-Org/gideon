#include "vm/vm.hpp"
#include "vm/instruction.hpp"

#include <sstream>

using namespace std;
using namespace raytrace;

/* Arithmetic Instructions */

template<typename T>
int add_C_C_exec(vm *svm, const int *args) {
  svm->get<T>(args[0]) = svm->constant<T>(args[1]) + svm->constant<T>(args[2]);
  return svm->pc() + 1;
}

template<typename T>
int add_R_C_exec(vm *svm, const int *args) {
  svm->get<T>(args[0]) = svm->get<T>(args[1]) + svm->constant<T>(args[2]);
  return svm->pc() + 1;
}

template<typename T>
int add_R_R_exec(vm *svm, const int *args) {
  svm->get<T>(args[0]) = svm->get<T>(args[1]) + svm->get<T>(args[2]);
  return svm->pc() + 1;
}

template<typename T>
int sub_C_C_exec(vm *svm, const int *args) {
  svm->get<T>(args[0]) = svm->constant<T>(args[1]) - svm->constant<T>(args[2]);
  return svm->pc() + 1;
}

template<typename T>
int sub_R_C_exec(vm *svm, const int *args) {
  svm->get<T>(args[0]) = svm->get<T>(args[1]) - svm->constant<T>(args[2]);
  return svm->pc() + 1;
}

template<typename T>
int sub_R_R_exec(vm *svm, const int *args) {
  svm->get<T>(args[0]) = svm->get<T>(args[1]) - svm->get<T>(args[2]);
  return svm->pc() + 1;
}

template<typename Scalar, typename Vector>
int mul_C_C_exec(vm *svm, const int *args) {
  svm->get<Vector>(args[0]) = svm->constant<Scalar>(args[1]) * svm->constant<Vector>(args[2]);
  return svm->pc() + 1;
}

template<typename Scalar, typename Vector>
int mul_R_C_exec(vm *svm, const int *args) {
  svm->get<Vector>(args[0]) = svm->get<Scalar>(args[1]) * svm->constant<Vector>(args[2]);
  return svm->pc() + 1;
}

template<typename Scalar, typename Vector>
int mul_C_R_exec(vm *svm, const int *args) {
  svm->get<Vector>(args[0]) = svm->constant<Scalar>(args[1]) * svm->get<Vector>(args[2]);
  return svm->pc() + 1;
}

template<typename Scalar, typename Vector>
int mul_R_R_exec(vm *svm, const int *args) {
  svm->get<Vector>(args[0]) = svm->get<Scalar>(args[1]) * svm->get<Vector>(args[2]);
  return svm->pc() + 1;
}

template<typename T>
int elem_mul_C_C_exec(vm *svm, const int *args) {
  svm->get<T>(args[0]) = svm->constant<T>(args[1]) * svm->constant<T>(args[2]);
  return svm->pc() + 1;
}

template<typename T>
int elem_mul_R_C_exec(vm *svm, const int *args) {
  svm->get<T>(args[0]) = svm->get<T>(args[1]) * svm->constant<T>(args[2]);
  return svm->pc() + 1;
}

template<typename T>
int elem_mul_R_R_exec(vm *svm, const int *args) {
  svm->get<T>(args[0]) = svm->get<T>(args[1]) * svm->get<T>(args[2]);
  return svm->pc() + 1;
}

template<typename T>
int div_C_C_exec(vm *svm, const int *args) {
  svm->get<T>(args[0]) = svm->constant<T>(args[1]) / svm->constant<T>(args[2]);
  return svm->pc() + 1;
}

template<typename T>
int div_R_C_exec(vm *svm, const int *args) {
  svm->get<T>(args[0]) = svm->get<T>(args[1]) / svm->constant<T>(args[2]);
  return svm->pc() + 1;
}

template<typename T>
int div_R_R_exec(vm *svm, const int *args) {
  svm->get<T>(args[0]) = svm->get<T>(args[1]) / svm->get<T>(args[2]);
  return svm->pc() + 1;
}

//Vector Ops

template<typename Scalar, typename Vector>
int dot_C_C_exec(vm *svm, const int *args) {
  svm->get<Scalar>(args[0]) = dot(svm->constant<Vector>(args[1]),
				  svm->constant<Vector>(args[2]));
  return svm->pc() + 1;
}

template<typename Scalar, typename Vector>
int dot_R_C_exec(vm *svm, const int *args) {
  svm->get<Scalar>(args[0]) = dot(svm->get<Vector>(args[1]),
				  svm->constant<Vector>(args[2]));
  return svm->pc() + 1;
}

template<typename Scalar, typename Vector>
int dot_R_R_exec(vm *svm, const int *args) {
  svm->get<Scalar>(args[0]) = dot(svm->get<Vector>(args[1]),
				  svm->get<Vector>(args[2]));
  return svm->pc() + 1;
}

template<typename T>
int length_C_exec(vm *svm, const int *args) {
  svm->get<float>(args[0]) = length(svm->constant<T>(args[1]));
  return svm->pc() + 1;
}

template<typename T>
int length_R_exec(vm *svm, const int *args) {
  svm->get<float>(args[0]) = length(svm->get<T>(args[1]));
  return svm->pc() + 1;
}

template<typename T>
int normalize_C_exec(vm *svm, const int *args) {
  svm->get<T>(args[0]) = normalize(svm->constant<T>(args[1]));
  return svm->pc() + 1;
}

template<typename T>
int normalize_R_exec(vm *svm, const int *args) {
  svm->get<T>(args[0]) = normalize(svm->get<T>(args[1]));
  return svm->pc() + 1;
}

/* Instruction Creation */

template<typename T>
void create_add_instructions(instruction_set &instructions, const string &typecode) {
  stringstream ss_cc, ss_rc, ss_rr;
  ss_cc << "add_" << typecode << "C_" << typecode << "C";
  ss_rc << "add_" << typecode << "_" << typecode << "C";
  ss_rr << "add_" << typecode << "_" << typecode;

  instructions.add_instruction({ss_cc.str(), 3, add_C_C_exec<T>});
  instructions.add_instruction({ss_rc.str(), 3, add_R_C_exec<T>});
  instructions.add_instruction({ss_rr.str(), 3, add_R_R_exec<T>});
}

template<typename T>
void create_sub_instructions(instruction_set &instructions, const string &typecode) {
  stringstream ss_cc, ss_rc, ss_rr;
  ss_cc << "sub_" << typecode << "C_" << typecode << "C";
  ss_rc << "sub_" << typecode << "_" << typecode << "C";
  ss_rr << "sub_" << typecode << "_" << typecode;

  instructions.add_instruction({ss_cc.str(), 3, sub_C_C_exec<T>});
  instructions.add_instruction({ss_rc.str(), 3, sub_R_C_exec<T>});
  instructions.add_instruction({ss_rr.str(), 3, sub_R_R_exec<T>});
}

template<typename ScalarType, typename VectorType>
void create_mul_instructions(instruction_set &instructions,
			     const string &scalar_typecode, const string &vector_typecode) {
  stringstream ss_cc, ss_rc, ss_cr, ss_rr;
  ss_cc << "mul_" << scalar_typecode << "C_" << vector_typecode << "C";
  ss_rc << "mul_" << scalar_typecode << "_" << vector_typecode << "C";
  ss_cr << "mul_" << scalar_typecode << "C_" << vector_typecode;
  ss_rr << "mul_" << scalar_typecode << "_" << vector_typecode;

  instructions.add_instruction({ss_cc.str(), 3, mul_C_C_exec<ScalarType, VectorType>});
  instructions.add_instruction({ss_rc.str(), 3, mul_R_C_exec<ScalarType, VectorType>});
  instructions.add_instruction({ss_cr.str(), 3, mul_C_R_exec<ScalarType, VectorType>});
  instructions.add_instruction({ss_rr.str(), 3, mul_R_R_exec<ScalarType, VectorType>});
}

template<typename T>
void create_elem_mul_instructions(instruction_set &instructions, const string &typecode) {
  stringstream ss_cc, ss_rc, ss_rr;
  ss_cc << "elem_mul_" << typecode << "C_" << typecode << "C";
  ss_rc << "elem_mul_" << typecode << "_" << typecode << "C";
  ss_rr << "elem_mul_" << typecode << "_" << typecode;

  instructions.add_instruction({ss_cc.str(), 3, elem_mul_C_C_exec<T>});
  instructions.add_instruction({ss_rc.str(), 3, elem_mul_R_C_exec<T>});
  instructions.add_instruction({ss_rr.str(), 3, elem_mul_R_R_exec<T>});

}

template<typename T>
void create_div_instructions(instruction_set &instructions, const string &typecode) {
  stringstream ss_cc, ss_rc, ss_rr;
  ss_cc << "div_" << typecode << "C_" << typecode << "C";
  ss_rc << "div_" << typecode << "_" << typecode << "C";
  ss_rr << "div_" << typecode << "_" << typecode;

  instructions.add_instruction({ss_cc.str(), 3, div_C_C_exec<T>});
  instructions.add_instruction({ss_rc.str(), 3, div_R_C_exec<T>});
  instructions.add_instruction({ss_rr.str(), 3, div_R_R_exec<T>});
}

template<typename T>
void create_dot_instructions(instruction_set &instructions, const string &typecode) {
  stringstream ss_cc2, ss_rc2, ss_rr2;
  ss_cc2 << "dot_" << typecode << "2C_" << typecode << "2C";
  ss_rc2 << "dot_" << typecode << "2_" << typecode << "2C";
  ss_rr2 << "dot_" << typecode << "2_" << typecode << "2";

  stringstream ss_cc3, ss_rc3, ss_rr3;
  ss_cc3 << "dot_" << typecode << "3C_" << typecode << "3C";
  ss_rc3 << "dot_" << typecode << "3_" << typecode << "3C";
  ss_rr3 << "dot_" << typecode << "3_" << typecode << "3";

  stringstream ss_cc4, ss_rc4, ss_rr4;
  ss_cc4 << "dot_" << typecode << "4C_" << typecode << "4C";
  ss_rc4 << "dot_" << typecode << "4_" << typecode << "4C";
  ss_rr4 << "dot_" << typecode << "4_" << typecode << "4";
  
  instructions.add_instruction({ss_cc2.str(), 3, dot_C_C_exec<T, gen_vec2<T> >});
  instructions.add_instruction({ss_rc2.str(), 3, dot_R_C_exec<T, gen_vec2<T> >});
  instructions.add_instruction({ss_rr2.str(), 3, dot_R_R_exec<T, gen_vec2<T> >});

  instructions.add_instruction({ss_cc3.str(), 3, dot_C_C_exec<T, gen_vec3<T> >});
  instructions.add_instruction({ss_rc3.str(), 3, dot_R_C_exec<T, gen_vec3<T> >});
  instructions.add_instruction({ss_rr3.str(), 3, dot_R_R_exec<T, gen_vec3<T> >});

  instructions.add_instruction({ss_cc4.str(), 3, dot_C_C_exec<T, gen_vec4<T> >});
  instructions.add_instruction({ss_rc4.str(), 3, dot_R_C_exec<T, gen_vec4<T> >});
  instructions.add_instruction({ss_rr4.str(), 3, dot_R_R_exec<T, gen_vec4<T> >});
}

void create_vector_ops(instruction_set &instr) {
  instr.add_instruction({"length_f2C", 2, length_C_exec<float2>});
  instr.add_instruction({"length_f2", 2, length_R_exec<float2>});

  instr.add_instruction({"length_f3C", 2, length_C_exec<float3>});
  instr.add_instruction({"length_f3", 2, length_R_exec<float3>});

  instr.add_instruction({"length_f4C", 2, length_C_exec<float4>});
  instr.add_instruction({"length_f4", 2, length_R_exec<float4>});

  instr.add_instruction({"normalize_f2C", 2, normalize_C_exec<float2>});
  instr.add_instruction({"normalize_f2", 2, normalize_R_exec<float2>});

  instr.add_instruction({"normalize_f3C", 2, normalize_C_exec<float3>});
  instr.add_instruction({"normalize_f3", 2, normalize_R_exec<float3>});

  instr.add_instruction({"normalize_f4C", 2, normalize_C_exec<float4>});
  instr.add_instruction({"normalize_f4", 2, normalize_R_exec<float4>});
}

void create_arithmetic_instructions(instruction_set &instr) {
  create_add_instructions<int>(instr, "i");
  create_add_instructions<float>(instr, "f");
  create_add_instructions<float2>(instr, "f2");
  create_add_instructions<float3>(instr, "f3");
  create_add_instructions<float4>(instr, "f4");

  create_sub_instructions<int>(instr, "i");
  create_sub_instructions<float>(instr, "f");
  create_sub_instructions<float2>(instr, "f2");
  create_sub_instructions<float3>(instr, "f3");
  create_sub_instructions<float4>(instr, "f4");

  create_div_instructions<int>(instr, "i");
  create_div_instructions<float>(instr, "f");

  create_dot_instructions<int>(instr, "i");
  create_dot_instructions<float>(instr, "f");

  create_vector_ops(instr);

  create_mul_instructions<int, int>(instr, "i", "i");
  create_mul_instructions<float, float>(instr, "f", "f");
  create_mul_instructions<float, float2>(instr, "f", "f2");
  create_mul_instructions<float, float3>(instr, "f", "f3");
  create_mul_instructions<float, float4>(instr, "f", "f4");

  create_elem_mul_instructions<int2>(instr, "i2");
  create_elem_mul_instructions<int3>(instr, "i3");
  create_elem_mul_instructions<int4>(instr, "i4");
  create_elem_mul_instructions<float2>(instr, "f2");
  create_elem_mul_instructions<float3>(instr, "f3");
  create_elem_mul_instructions<float4>(instr, "f4");
}
