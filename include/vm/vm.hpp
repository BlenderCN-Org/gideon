#ifndef RT_VM_HPP
#define RT_VM_HPP

#include "math/vector.hpp"
#include "math/random.hpp"

#include "vm/registers.hpp"
#include "vm/program.hpp"
#include "vm/parameters.hpp"

namespace raytrace {

  struct scene;
  struct instruction;
  struct bvh;

  //A virtual mission in which all raytrace-related programs execute.
  class vm {
  public:
    vm(scene *s, bvh *accel) : s(s), accel(accel),
			       register_stack(16, register_file(128)), stack_frame(0),
			       execution_stack(16), execution_frame(0)
    {}
        
    /*
      Executes the provided program. Execution starts at the instruction designated by entry_point.
      The caller may also pass in a set of input parameters and output values.

      The VM maintains a stack of all programs currently being executed, so it is possible for
      programs to execute other programs. The VM then doesn't care if it's executing an integrator,
      shader or distribution program.
    */
    void execute(program &prog, unsigned int entry_point,
		 parameter_list *input_vars, parameter_list *output_vars);
    
    scene *s;
    bvh *accel;
    
    struct output_target {
      float *rgba_out;
      unsigned int pixel_idx;
    };

    output_target output;
    
    /* VM-Access functions that Instructions may use. */

    template<typename T>
    T &get(int i) {
      int stack_idx;
      int register_idx = find_register(i, register_stack[stack_frame].capacity<T>(), stack_idx);

      return register_stack[stack_idx].get<T>(register_idx);
    }

    template<typename T>
    T &constant(int i) { return current_program()->constants.get<T>(i); }

    template<typename T>
    T &input_param(int offset) { return inputs()->get<T>(offset); }

    template<typename T>
    T &output_param(int offset) { return outputs()->get<T>(offset); }
    
    template<typename T>
    int frame_size() const { return register_stack[stack_frame].capacity<T>(); }
    
    void push_stack_frame();
    void pop_stack_frame();
    
    int pc();
    program *current_program();
    parameter_list *inputs();
    parameter_list *outputs();
    
    float random();

  private:
    
    std::vector<register_file> register_stack;
    unsigned int stack_frame;
    
    int find_register(int idx, int capacity, /* out */ int &stack_idx) const;    

    struct program_context {
      int pc;
      program *prog;
      parameter_list *input_vars, *output_vars;
    };

    std::vector<program_context> execution_stack;
    unsigned int execution_frame;

    random_number_gen rng;
  };

};

#endif
