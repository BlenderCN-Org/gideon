#include "vm/vm.hpp"

#include "vm/instruction.hpp"
#include "vm/program.hpp"
#include "scene/scene.hpp"

#include <stdexcept>
#include <iostream>

using namespace std;
using namespace raytrace;

void raytrace::vm::execute(program &prog, unsigned int entry_point,
			   parameter_list *input_vars, parameter_list *output_vars) {
  //push this program onto the execution stack
  unsigned int next_frame = execution_frame + 1;
  if (next_frame >= execution_stack.size()) throw runtime_error("Maximum program depth exceeded");
  execution_frame = next_frame;

  program_context &ctx = execution_stack[execution_frame];
  ctx.pc = entry_point;
  ctx.prog = &prog;
  ctx.input_vars = input_vars;
  ctx.output_vars = output_vars;

  //execute the program
  while (ctx.pc >= 0 && ctx.pc < prog.code.size()) {
    instruction &i = prog.code[ctx.pc];
    ctx.pc = (*i.is)[i.id].execute(this, i.args);
  }

  //pop it off the stack
  execution_frame--;
}

int raytrace::vm::find_register(int idx, int capacity,
				/* out */ int &stack_idx) const {
  stack_idx = stack_frame + (idx / capacity) - (idx < 0 ? 1 : 0);
  if (stack_idx < 0 || stack_idx >= register_stack.size()) throw runtime_error("Invalid stack frame");
						
  while (idx < 0) idx += capacity;
  int register_idx = (idx % capacity);
  //cout << "Access: [" << stack_idx << ":" << register_idx << "]" << endl;
  return register_idx;
}

void raytrace::vm::push_stack_frame() {
  unsigned int next_frame = stack_frame + 1;
  //cout << "Register Stack Size: " << register_stack.size() << endl;
  //cout << "Next Frame: " << next_frame << endl;
  if (next_frame >= register_stack.size()) throw runtime_error("Maximum stack depth exceeded");
  stack_frame = next_frame;
}

void raytrace::vm::pop_stack_frame() {
  //cout << "Popping stack frame!" << endl;
  if (stack_frame == 0) throw runtime_error("No stack frames to pop");
  stack_frame--;
}

int raytrace::vm::pc() { return execution_stack[execution_frame].pc; }

program *raytrace::vm::current_program() { return execution_stack[execution_frame].prog; }

parameter_list *raytrace::vm::inputs() {
  parameter_list *in = execution_stack[execution_frame].input_vars;
  if (!in) throw runtime_error("No input parameters to access");
  return in;
}

parameter_list *raytrace::vm::outputs() { 
  parameter_list *out = execution_stack[execution_frame].output_vars;
  if (!out) throw runtime_error("No output parameters to access");
  return out;
}

float raytrace::vm::random() {
  return rng();
}
