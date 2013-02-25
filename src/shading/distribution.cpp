#include "shading/distribution.hpp"

#include "scene/scene.hpp"

#include "vm/vm.hpp"
#include "vm/program.hpp"

#include <iostream>

using namespace std;
using namespace raytrace;

raytrace::distribution_function::distribution_function(const string &name, int flags,
						       program *p, unsigned int eval, unsigned int sample,
						       const map<string, size_t> &params) :
  flags(flags),
  name(name),
  prog(p),
  eval_entry(eval), sample_entry(sample),
  plist(params)
{
  
}

//Returns the value of this function at the given point.
float4 raytrace::distribution_function::evaluate(vm *svm, parameter_list *params,
						 const float3 &N,
						 const float3 &P_in, const float3 &P_out,
						 const float3 &w_in, const float3 &w_out) {
  //setup the execution environment for this function
  svm->push_stack_frame();

  //copy data into registers:
  svm->get<float3>(0) = N;
  svm->get<float3>(1) = P_in;
  svm->get<float3>(2) = w_in;
  svm->get<float3>(3) = P_out;
  svm->get<float3>(4) = w_out;
  
  svm->execute(*prog, eval_entry,
	       params, NULL);

  //read the value from the first float4 register
  float4 result = svm->get<float4>(0);
  
  //clean up
  svm->pop_stack_frame();

  return result;
}

size_t raytrace::distribution_function::param_size() const {
  size_t bytes = 0;
  for (map<string, size_t>::const_iterator it = plist.begin(); it != plist.end(); it++) {
    bytes += it->second;
  }
  
  return bytes;
}

raytrace::distribution_instance::distribution_instance(scene *s, int func_id) :
  id(func_id),
  params(s->distributions[func_id].param_size())
{
  
}
