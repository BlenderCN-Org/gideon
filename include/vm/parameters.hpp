#ifndef RT_VM_PARAMETERS_HPP
#define RT_VM_PARAMETERS_HPP

#include <vector>

namespace raytrace {

  /* Generic container for a program's input/output parameters. */
  class parameter_list {
  public:
    
    parameter_list(unsigned int size);
    
    template<typename T>
    T &get(int byte_offset) {
      return *reinterpret_cast<T*>(&data[0] + byte_offset);
    }

  private:

    std::vector<char> data;

  };

};

#endif
