#include "math/vector.hpp"
#include "vm/vm.hpp"
#include "vm/instruction.hpp"

#include "scene/scene.hpp"

#include <math.h>

#include <iostream>
#include <sstream>

using namespace std;
using namespace raytrace;

/* Debugging Instructions */

template<typename T>
int debug_to_string_exec(vm *svm, const int *args) {
  stringstream ss;
  ss << svm->get<T>(args[1]);
  svm->get<string>(args[0]) = ss.str();
  return svm->pc() + 1;
}

int string_concat_exec(vm *svm, const int *args) {
  svm->get<string>(args[0]) = svm->get<string>(args[1]) + svm->get<string>(args[2]);
  return svm->pc() + 1;
}

int debug_write_depth_exec(vm *svm, const int *args) {
  float d = expf(-0.2f*svm->get<float>(args[0]));
  float *rgba_out = svm->output.rgba_out;
  rgba_out[svm->output.pixel_idx++] = d;
  rgba_out[svm->output.pixel_idx++] = d;
  rgba_out[svm->output.pixel_idx++] = d;
  rgba_out[svm->output.pixel_idx++] = 1.0f;
  return svm->pc() + 1;
}

int debug_write_color_exec(vm *svm, const int *args) {
  float3 &c = svm->get<float3>(args[0]);
  float *rgba_out = svm->output.rgba_out;
  rgba_out[svm->output.pixel_idx++] = c.x;
  rgba_out[svm->output.pixel_idx++] = c.y;
  rgba_out[svm->output.pixel_idx++] = c.z;
  rgba_out[svm->output.pixel_idx++] = 1.0f;
  return svm->pc() + 1;
}

int debug_print_exec(vm *svm, const int *args) {
  cout << "[" << svm->get<int>(args[0]) << ", " << svm->get<int>(args[1]) << "]" << endl;
  return svm->pc() + 1;
}

int debug_print_str_exec(vm *svm, const int *args) {
  cout << svm->get<string>(args[0]) << endl;
  return svm->pc() + 1;
}

int debug_oren_nayar_exec(vm *svm, const int *args) {
  float sigma = svm->get<float>(args[1]);
  float sigma2 = sigma*sigma;

  float A = 1.0f - 0.5f * (sigma2 / (sigma2 + 0.33f));
  float B = 0.45f * (sigma2 / (sigma2 + 0.09f));

  float3 &N = svm->get<float3>(args[2]);
  float3 &w_in = svm->get<float3>(args[3]);
  float3 &w_out = svm->get<float3>(args[4]);

  float theta_in = acosf(dot(N, w_in));
  float theta_out = acosf(dot(N, w_out));

  float alpha = max(theta_in, theta_out);
  float beta = min(theta_in, theta_out);

  float3 t_in = normalize(w_in - N);
  float3 t_out = normalize(w_out - N);
  float cos_dphi = dot(t_in, t_out);

  svm->get<float>(args[0]) = dot(N, w_in) * (A + (B * max(0.0f, cos_dphi) * sinf(alpha) * tanf(beta)));
  return svm->pc() + 1;
}

/* Instruction Creation */

void create_string_conv_instructions(instruction_set &instr) {
  instr.add_instruction({"debug_to_string_i", 2, debug_to_string_exec<int>});
  instr.add_instruction({"debug_to_string_f", 2, debug_to_string_exec<float>});
}

void create_debug_instructions(instruction_set &instr) {
  create_string_conv_instructions(instr);

  instr.add_instruction({"string_concat", 3, string_concat_exec});

  instr.add_instruction({"debug_write_depth", 1, debug_write_depth_exec});
  instr.add_instruction({"debug_write_color", 1, debug_write_color_exec});  
  instr.add_instruction({"debug_print", 2, debug_print_exec});
  instr.add_instruction({"debug_print_str", 1, debug_print_str_exec});

  instr.add_instruction({"debug_oren_nayar", 5, debug_oren_nayar_exec});
}
