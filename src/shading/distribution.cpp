#include "shading/distribution.hpp"

#include "scene/scene.hpp"

#include <iostream>

using namespace std;
using namespace raytrace;

/* Shade-Tree Evaluation */

shade_tree::leaf::leaf(char *p, eval_func_type eval, sample_func_type sample, dtor_func_type dtor) : 
  params(p), evaluate(eval), sample(sample), destructor(dtor)
{
  
}

shade_tree::leaf::~leaf() {
  destructor(params);
  delete[] params;
}

/* Helper class that evaluates a shade_tree */
class shade_tree_evaluator : public boost::static_visitor<> {
public:
  
  float4 *L_in;
  float3 *P_in, *w_in;
  float3 *P_out, *w_out;
  /* out */ float *pdf;
  /* out */ float4 *out;

  shade_tree_evaluator(float4 *L_in,
		       float3 *P_in, float3 *w_in,
		       float3 *P_out, float3 *w_out, 
		       /* out */ float *pdf, /* out */ float4 *out) :
    L_in(L_in), P_in(P_in), w_in(w_in), P_out(P_out), w_out(w_out), pdf(pdf), out(out)
  {
    
  }

  void operator()(shade_tree::leaf_ptr &node) const {
    node->evaluate(node->params, L_in, P_in, w_in, P_out, w_out, pdf, out);
  }
  
  void operator()(shade_tree::scale_ptr &node) const {
    boost::apply_visitor(shade_tree_evaluator(L_in, P_in, w_in, P_out, w_out, pdf, out), node->node);
    *out = node->k * (*out);
  }

  void operator()(shade_tree::sum_ptr &node) const {
    float4 lhs, rhs;
    boost::apply_visitor(shade_tree_evaluator(L_in, P_in, w_in, P_out, w_out, pdf, &lhs), node->lhs);
    boost::apply_visitor(shade_tree_evaluator(L_in, P_in, w_in, P_out, w_out, pdf, &rhs), node->rhs);
    
    *out = lhs + rhs;
  }

};

void shade_tree::evaluate(node_ptr &node,
			  float4 *L_in,
			  float3 *P_in, float3 *w_in,
			  float3 *P_out, float3 *w_out,
			  /* out */ float *pdf, /* out */ float4 *out) {
  boost::apply_visitor(shade_tree_evaluator(L_in, P_in, w_in, P_out, w_out, pdf, out), node);
}

/* Shade Tree Sampling */

class shade_tree_sampler : public boost::static_visitor<float> {

public:

  float3 *P_out, *w_out;
  float2 *rand_P, *rand_w;
  /* out */ float3 *P_in, *w_in;
  
  shade_tree_sampler(float3 *P_out, float3 *w_out,
		     float2 *rand_P, float2 *rand_w,
		     /* out */ float3 *P_in, /* out */ float3 *w_in) :
    P_out(P_out), w_out(w_out), rand_P(rand_P), rand_w(rand_w), P_in(P_in), w_in(w_in)
  {
    
  }

  float operator()(shade_tree::leaf_ptr &node) const {
    return node->sample(node->params, P_out, w_out, rand_P, rand_w, P_in, w_in);
  }

  float operator()(shade_tree::scale_ptr &node) const {
    return boost::apply_visitor(shade_tree_sampler(P_out, w_out, rand_P, rand_w, P_in, w_in),
				node->node);
  }

  float operator()(shade_tree::sum_ptr &node) const {
    throw runtime_error("Sampling sums of distributions is not yet implemented.");
  }

};

float shade_tree::sample(node_ptr &node,
			 float3 *P_out, float3 *w_out,
			 float2 *rand_P, float2 *rand_w,
			 /* out */ float3 *P_in, /* out */ float3 *w_in) {
  return boost::apply_visitor(shade_tree_sampler(P_out, w_out, rand_P, rand_w, P_in, w_in),
			      node);						 
}
