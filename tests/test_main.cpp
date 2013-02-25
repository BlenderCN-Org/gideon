#include "vm/vm.hpp"
#include "vm/instruction.hpp"
#include "vm/standard.hpp"
#include "scene/scene.hpp"

#include <iostream>

using namespace std;
using namespace raytrace;

void test_range(vm &svm, instruction_set &is, int start, int end) {
  svm.constants.get<int>(0) = 1;
  svm.constants.get<int>(1) = start;
  svm.constants.get<int>(2) = end;

  vector<instruction> prog;
  assemble(is, prog,
	   vector<string>{
	     "mov_i_iC 0 1",
	       "sub_i_iC 1 0 2",
	       "jump_zero 1 5",
	       "add_i_iC 0 0 0",
	       "jump 1"
	       });
  
  //execute the program
  while (svm.pc < prog.size()) {
    cout << "PC: " << svm.pc << endl;
    svm.execute(prog[svm.pc]);
  }

  cout << "Loop Counter: " << svm.get<int>(0) << endl;
}

void sample_pixels(vm &svm, instruction_set &is, int x_pixels, int y_pixels) {
  svm.constants.get<float>(0) = x_pixels;
  svm.constants.get<float>(1) = y_pixels;
  svm.constants.get<float>(2) = 1.0f;
  svm.constants.get<float>(3) = 0.0f;

  svm.constants.get<int>(0) = 0;
  svm.constants.get<int>(1) = x_pixels;
  svm.constants.get<int>(2) = y_pixels;
  svm.constants.get<int>(3) = 1;

  vector<instruction> prog;
  assemble(is, prog,
	   {
	     //compute pixel dx/dy
	     //dx = 1.0 / x_pixels  |  dy = 1.0 / y_pixels
	     "div_fC_fC 0 2 0", //r0 = dx
	       "div_fC_fC 1 2 1", //r1 = dy
	       
	       //setup x loop counter
	       "mov_f_fC 2 3", //x = 0.0
	       "mov_i_iC 0 0", //i = 0

	       //setup y loop
	       "mov_f_fC 3 3", //y = 0.0
	       "mov_i_iC 1 0", //j = 0

	       //increment y
	       "add_f_f 3 1 3", //y += dy
	       "add_i_iC 1 1 3", //j += 1
	       
	       //y loop
	       "sub_i_iC 2 1 2", //tmp = j - y_pixels
	       "jump_nz 2 6", //continue if j != y_pixels
	       
	       //increment x
	       "add_f_f 2 0 2", //x += dx
	       "add_i_iC 0 0 3", //i += 1

	       //x loop
	       "sub_i_iC 2 0 1", //tmp = i - x_pixels
	       "jump_nz 2 4" //continue if i != x_pixels
	       });

  while (svm.pc < prog.size()) {
    svm.execute(prog[svm.pc]);
  }

  cout << "Dx: " << svm.get<float>(0) << " | Dy: " << svm.get<float>(1) << endl;
  cout << "Pixels: [" << svm.get<int>(0) << ", " << svm.get<int>(1) << "]" << endl;
}

void func_call(vm &svm, instruction_set &is) {
  svm.constants.get<float>(0) = 13.0f;
  svm.constants.get<float>(1) = 2.3f;
  
  svm.constants.get<int>(0) = 1;
  
  vector<instruction> prog;
  assemble(is, prog,
	   {
	     "jump 3", //jump to start

	       //f(x) = x + 13
	       "add_f_fC -1 0 0", 
	       "jump_R 1", //return

	       //start
	       "mov_f_fC 128 1", //set the arguments
	       "mov_i_iC 128 0", //set the function location

	       //function call
	       "stack_push",
	       "save_pc_offset 1 2",
	       "jump_R 0",
	       "stack_pop",

	       "mov_f_f 0 127"
	       });

  while (svm.pc < prog.size()) { 
    cout << "PC: " << svm.pc << endl;
    svm.execute(prog[svm.pc]);
  }

  cout << "Result: " << svm.get<float>(0) << endl;
}

int main(int argc, char **argv) {
  vm svm(NULL, NULL);
  instruction_set is = create_standard_instruction_set();

  attribute_type t = get_attribute_type<int3>();
  attribute_type ref = {attribute_type::FLOAT, attribute_type::SCALAR};
  cout << "T: " << t.base_type << " | " << ref.base_type << endl;
  cout << "Agg: " << t.aggregate_type << " | " << ref.aggregate_type << endl;
  
  /*instruction i0 = is.make("add_f3C_f3C", 0, 0, 1);
  instruction i1 = is.make("mul_fC_f3", 0, 1, 0);

  cout << "Float3 Capacity: " << svm.constants.capacity<float3>() << endl;
  cout << "Ray Capacity: " << svm.registers.capacity<ray>() << endl;

  svm.constants.get<float3>(0) = float3{0.0f, 0.0f, 4.0f};
  svm.constants.get<float3>(1) = float3{0.0f, 2.0f, 3.0f};
  svm.constants.get<float>(1) = 0.5f;

  cout << "Executing Instruction " << i0.id << "... " << endl;
  svm.execute(i0);
  svm.execute(i1);
  cout << "Done." << endl;

  float3 &v = svm.registers.get<float3>(0);
  cout << "F3_0: {" << v.x << ", " << v.y << ", " << v.z << "}" << endl;*/
  
  //test_range(svm, is, 0, 4);
  //sample_pixels(svm, is, 3, 3);
  func_call(svm, is);
  
  return 0;
}
