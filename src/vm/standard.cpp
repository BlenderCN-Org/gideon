#include "vm/standard.hpp"
#include "vm/vm.hpp"

using namespace std;
using namespace raytrace;

extern void create_arithmetic_instructions(instruction_set &instr);
extern void create_control_instructions(instruction_set &instr);
extern void create_data_instructions(instruction_set &instr);
extern void create_attribute_instructions(instruction_set &instr);
extern void create_query_instructions(instruction_set &instr);
extern void create_geometry_instructions(instruction_set &instr);
extern void create_debug_instructions(instruction_set &instr);
extern void create_shading_instructions(instruction_set &instr);

/* Instruction Set Creation */

instruction_set raytrace::create_standard_instruction_set() {
  instruction_set std_set{"standard"};

  create_arithmetic_instructions(std_set);
  create_data_instructions(std_set);
  create_control_instructions(std_set);
  create_attribute_instructions(std_set);
  create_geometry_instructions(std_set);
  create_shading_instructions(std_set);
  create_query_instructions(std_set);
  create_debug_instructions(std_set);

  return std_set;
}
