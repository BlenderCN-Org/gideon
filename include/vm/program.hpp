#ifndef RT_PROGRAM_HPP
#define RT_PROGRAM_HPP

#include "vm/registers.hpp"
#include "vm/instruction.hpp"

namespace raytrace {
  
  /*
    A piece of code that can be executed.
    Contains a register file for program-specific constant data.
  */
  class program {
  public:
    
    program() : constants(128) {}
    
    register_file constants;
    std::vector<instruction> code;
    
  };

};

#endif
