#include "vm/instruction.hpp"

#include <iostream>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

using namespace raytrace;
using namespace std;

void raytrace::instruction_set::add_instruction(const instruction_desc &instr) {
  int idx = static_cast<int>(instruction_list.size());

  instruction_list.push_back(instr);
  instruction_lookup[instr.name] = idx;
}

int raytrace::instruction_set::lookup(const string &name) const {
  map<string, int>::const_iterator it = instruction_lookup.find(name);
  if (it == instruction_lookup.end()) return -1;
  return it->second;
}

const instruction_desc &raytrace::instruction_set::operator[](int i) const {
  return instruction_list[i];
}

instruction raytrace::instruction_set::make(const std::string &name, unsigned int num_args,
					    int arg0, int arg1, int arg2, int arg3,
					    int arg4, int arg5, int arg6, int arg7) const {
  int id = lookup(name);
  if (id < 0) throw runtime_error(string("Invalid instruction: ") + name);
  if (instruction_list[id].num_args != num_args) {
    stringstream err_str;
    err_str << "Invalid Argument Count - " << name << ": Expected " << instruction_list[id].num_args << ", found " << num_args;
    throw runtime_error(err_str.str());
  }

  return {this, lookup(name), {arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7}};
}

namespace raytrace {

  instruction instruction_from_string(const instruction_set &is, const string &s) {
    vector<string> args;
    boost::split(args, s, boost::is_any_of(" "));

    int r[8];
    if (args.size() < 1) throw runtime_error("Empty instruction");

    unsigned int num_args = args.size() - 1;
    
    try {
      for (unsigned int i = 1; i < args.size(); i++) {
	r[i - 1] = boost::lexical_cast<int>(args[i]);
      }
    }
    catch (exception &e) {
      cerr << "Error with string: " << s << endl;
      throw;
    }
    
    return is.make(args[0], num_args, r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7]);
  }
  
  void assemble(const instruction_set &is, /* out */ vector<instruction> &prog,
		const vector<string> &code) {
    for (auto it = code.begin(); it != code.end(); it++) {
      prog.push_back(raytrace::instruction_from_string(is, *it));
    }
  }
  
};
