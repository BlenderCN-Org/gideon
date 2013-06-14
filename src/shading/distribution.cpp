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

shade_tree::scale::scale(const float4 &k, const node_ptr &node) :
  k(k), node(node), weight(length(k)*get_weight(node))
{
  
}

shade_tree::sum::sum(const node_ptr &lhs, const node_ptr &rhs) :
  lhs(lhs), rhs(rhs),
  weight(get_weight(lhs) + get_weight(rhs))
{
  
}

/* Helper class that evaluates a shade_tree */
class shade_tree_evaluator : public boost::static_visitor<> {
public:
  
  float3 *P_in, *w_in;
  float3 *P_out, *w_out;
  /* out */ float *pdf;
  /* out */ float4 *out;

  shade_tree_evaluator(float3 *P_in, float3 *w_in,
		       float3 *P_out, float3 *w_out, 
		       /* out */ float *pdf, /* out */ float4 *out) :
    P_in(P_in), w_in(w_in), P_out(P_out), w_out(w_out), pdf(pdf), out(out)
  {
    
  }

  void operator()(shade_tree::leaf_ptr &node) const {
    node->evaluate(node->params, P_in, w_in, P_out, w_out, pdf, out);
  }
  
  void operator()(shade_tree::scale_ptr &node) const {
    boost::apply_visitor(shade_tree_evaluator(P_in, w_in, P_out, w_out, pdf, out), node->node);
    *out = node->k * (*out);
  }

  void operator()(shade_tree::sum_ptr &node) const {
    float4 lhs, rhs;
    float p_L = get_weight(node->lhs) / node->weight;
    float p_R = 1.0f - p_L;
    float pdf0, pdf1;

    boost::apply_visitor(shade_tree_evaluator(P_in, w_in, P_out, w_out, &pdf0, &lhs), node->lhs);
    boost::apply_visitor(shade_tree_evaluator(P_in, w_in, P_out, w_out, &pdf1, &rhs), node->rhs);
    
    *out = lhs + rhs;
    *pdf = (p_L*pdf0 + p_R*pdf1);
  }

};

void shade_tree::evaluate(node_ptr &node,
			  float3 *P_in, float3 *w_in,
			  float3 *P_out, float3 *w_out,
			  /* out */ float *pdf, /* out */ float4 *out) {
  boost::apply_visitor(shade_tree_evaluator(P_in, w_in, P_out, w_out, pdf, out), node);
}

/* Node Weight Computation */

class shade_tree_weight : public boost::static_visitor<float> {
public:

  float operator()(const shade_tree::leaf_ptr &node) const { return 1.0f; }
  float operator()(const shade_tree::scale_ptr &node) const { return node->weight; }
  float operator()(const shade_tree::sum_ptr &node) const { return node->weight; }
  
};

float shade_tree::get_weight(const node_ptr &node) {
  return boost::apply_visitor(shade_tree_weight(), node);
}

/* Shade Tree Sampling */

class shade_tree_sampler : public boost::static_visitor<float> {

public:

  float3 *P_out, *w_out;
  float rand_D; //expected to be scaled by the tree's total weight
  float2 *rand_P, *rand_w;
  /* out */ float3 *P_in, *w_in;
  
  shade_tree_sampler(float3 *P_out, float3 *w_out,
		     float rand_D, float2 *rand_P, float2 *rand_w,
		     /* out */ float3 *P_in, /* out */ float3 *w_in) :
    P_out(P_out), w_out(w_out),
    rand_D(rand_D), rand_P(rand_P), rand_w(rand_w),
    P_in(P_in), w_in(w_in)
  {
    
  }

  float operator()(shade_tree::leaf_ptr &node) const {
    return node->sample(node->params, P_out, w_out, rand_P, rand_w, P_in, w_in);
  }

  float operator()(shade_tree::scale_ptr &node) const {
    return boost::apply_visitor(shade_tree_sampler(P_out, w_out, rand_D, rand_P, rand_w, P_in, w_in),
				node->node);
  }

  float operator()(shade_tree::sum_ptr &node) const {
    //select which argument to sample (based on weight)
    float w_L = get_weight(node->lhs);
    float w_R = get_weight(node->rhs);
    float pdf_L = w_L / node->weight;
    float pdf_R = 1.0f - pdf_L;
    

    if (rand_D < w_L) {
      return pdf_R * boost::apply_visitor(shade_tree_sampler(P_out, w_out, rand_D, rand_P, rand_w, P_in, w_in),
					  node->lhs);
    }
    
    return pdf_L * boost::apply_visitor(shade_tree_sampler(P_out, w_out, rand_D - w_L, rand_P, rand_w, P_in, w_in),
					node->rhs);
  }

};

float shade_tree::sample(node_ptr &node,
			 float3 *P_out, float3 *w_out,
			 float rand_D, float2 *rand_P, float2 *rand_w,
			 /* out */ float3 *P_in, /* out */ float3 *w_in) {
  float D = get_weight(node) * rand_D;
  return boost::apply_visitor(shade_tree_sampler(P_out, w_out, D, rand_P, rand_w, P_in, w_in),
			      node);						 
}
