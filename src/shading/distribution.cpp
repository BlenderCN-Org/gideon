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

/* Shade-Tree Evaluation */

shade_tree::leaf::leaf(char *p, eval_func_type eval, dtor_func_type dtor) : 
  params(p), evaluate(eval), destructor(dtor)
{
  
}

shade_tree::leaf::~leaf() {
  destructor(params);
  delete[] params;
}

/* Helper class that evaluates a shade_tree */
class shade_tree_evaluator : public boost::static_visitor<> {
public:
  
  float3 *N;
  float3 *P_in, *w_in;
  float3 *P_out, *w_out;
  /* out */ float4 *out;

  shade_tree_evaluator(float3 *N,
		       float3 *P_in, float3 *w_in,
		       float3 *P_out, float3 *w_out, 
		       /* out */ float4 *out) :
    N(N), P_in(P_in), w_in(w_in), P_out(P_out), w_out(w_out), out(out)
  {
    
  }

  void operator()(shade_tree::leaf_ptr &node) const {
    node->evaluate(node->params, N, P_in, w_in, P_out, w_out, out);
  }
  
  void operator()(shade_tree::scale_ptr &node) const {
    boost::apply_visitor(shade_tree_evaluator(N, P_in, w_in, P_out, w_out, out), node->node);
    *out = node->k * (*out);
  }

  void operator()(shade_tree::sum_ptr &node) const {
    float4 lhs, rhs;
    boost::apply_visitor(shade_tree_evaluator(N, P_in, w_in, P_out, w_out, &lhs), node->lhs);
    boost::apply_visitor(shade_tree_evaluator(N, P_in, w_in, P_out, w_out, &rhs), node->rhs);
    
    *out = lhs + rhs;
  }

};

void shade_tree::evaluate(node_ptr &node,
			  float3 *N,
			  float3 *P_in, float3 *w_in,
			  float3 *P_out, float3 *w_out,
			  /* out */ float4 *out) {
  boost::apply_visitor(shade_tree_evaluator(N, P_in, w_in, P_out, w_out, out), node);
}
