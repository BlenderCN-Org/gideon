#ifndef RT_VM_INSTRUCTION_HPP
#define RT_VM_INSTRUCTION_HPP

#include <string>
#include <vector>
#include <map>

namespace raytrace {

  class vm;
  
  const unsigned int max_instruction_arguments = 8;

  struct instruction_desc {
    std::string name;
    unsigned int num_args;
   
    int (*execute)(vm *svm, const int *args);
  };

  struct instruction;

  struct instruction_set {
    std::string name;
    std::vector<instruction_desc> instruction_list;
    std::map<std::string, int> instruction_lookup;

    void add_instruction(const instruction_desc &instr);

    int lookup(const std::string &name) const;
    const instruction_desc &operator[](int i) const;

    instruction make(const std::string &name, unsigned int num_args,
		     int arg0 = 0, int arg1 = 0, int arg2 = 0, int arg3 = 0,
		     int arg4 = 0, int arg5 = 0, int arg6 = 0, int arg7 = 0) const;
  };

  struct instruction {
    const instruction_set *is;
    
    int id;
    int args[max_instruction_arguments];
  };
  
  instruction instruction_from_string(const instruction_set &is, const std::string &str);
  void assemble(const instruction_set &is, /* out */ std::vector<instruction> &program,
		const std::vector<std::string> &code);
};

#endif
