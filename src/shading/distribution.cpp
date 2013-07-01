#include "shading/distribution.hpp"

#include "scene/scene.hpp"
#include "math/sampling.hpp"

#include <iostream>

using namespace std;
using namespace raytrace;

/* Lead Node */

shade_tree::leaf::leaf(char *p, shader_flags flags,
		       eval_func_type eval, sample_func_type sample,
		       pdf_func_type pdf, emission_func_type(emit),
		       dtor_func_type dtor) : 
  params(p), evaluate_fn(eval), sample_fn(sample), pdf_fn(pdf), emit_fn(emit), destructor(dtor),
  flags(flags)
{
  
}

shade_tree::leaf::~leaf() {
  destructor(params);
  delete[] params;
}

void shade_tree::leaf::evaluate(float3 *P_in, float3 *w_in,
				float3 *P_out, float3 *w_out,
				/* out */ float *pdf, /* out */ float4 *eval) const {
  if (evaluate_fn) evaluate_fn(params, P_in, w_in, P_out, w_out, pdf, eval);
  else {
    *pdf = 0.25f * pi;
    *eval = {0.0f, 0.0f, 0.0f, 1.0f};
  }
}

float shade_tree::leaf::pdf(float3 *P_in, float3 *w_in,
			    float3 *P_out, float3 *w_out) const {
  if (pdf_fn) return pdf_fn(params, P_in, w_in, P_out, w_out);
  else if (evaluate_fn) {
    float pdf;
    float4 eval;
    
    evaluate_fn(params, P_in, w_in, P_out, w_out,
		&pdf, &eval);
    return pdf;
  }
  else return 0.25f * pi;
}

float shade_tree::leaf::sample(float3 *P_out, float3 *w_out,
			       float2 *rand_P, float2 *rand_w,
			       /* out */ float3 *P_in, /* out */ float3 *w_in) const {
  if (sample_fn) return sample_fn(params, P_out, w_out, rand_P, rand_w, P_in, w_in);
  else {
    //uniformly sample the sphere
    *P_in = *P_out;
    *w_in = uniform_sample_sphere((*rand_w)[0], (*rand_w)[1]);
    return 0.25f * pi;
  }
}

void shade_tree::leaf::emission(float3 *P_out, float3 *w_out, /* out */ float4 *Le) const {
  if (emit_fn) emit_fn(params, P_out, w_out, Le);
  else {
    *Le = {0.0f, 0.0f, 0.0f, 0.0f};
  }
}

/* Node Flags Computation */

class shader_flags_visitor : public boost::static_visitor<shade_tree::shader_flags> {
public:

  shade_tree::shader_flags operator()(const shade_tree::leaf_ptr &node) const { return node->get_flags(); }
  shade_tree::shader_flags operator()(const shade_tree::scale_ptr &node) const { return node->flags; }
  shade_tree::shader_flags operator()(const shade_tree::sum_ptr &node) const { return node->flags; }

};

shade_tree::shader_flags shade_tree::get_flags(const node_ptr &node) {
  return boost::apply_visitor(shader_flags_visitor(), node);
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

class weight_visitor : public boost::static_visitor<float> {
public:

  shade_tree::shader_flags mask;
  weight_visitor(shade_tree::shader_flags mask) : mask(mask) { }

  float operator()(const shade_tree::leaf_ptr &node) const { return 1.0f; }
  
  float operator()(const shade_tree::scale_ptr &node) const {
    bool accept = ((mask == 0) || (mask & get_flags(node->node)));
    return accept ? boost::apply_visitor(*this, node->node) : 0.0f;
  }
  
  float operator()(const shade_tree::sum_ptr &node) const {
    bool accept_L = ((mask == 0) || (mask & get_flags(node->lhs)));
    bool accept_R = ((mask == 0) || (mask & get_flags(node->rhs)));

    float weight_L = accept_L ? boost::apply_visitor(*this, node->lhs) : 0.0f;
    float weight_R = accept_L ? boost::apply_visitor(*this, node->rhs) : 0.0f;
    return weight_L + weight_R;
  }
};

float shade_tree::get_weight(shade_tree::shader_flags flags, const node_ptr &node) {
  return boost::apply_visitor(weight_visitor(flags), node);
}

/* Shade-Tree Evaluation */

shade_tree::scale::scale(const float4 &k, const node_ptr &node) :
  k(k), node(node), weight(length(k)*get_weight(node)), flags(get_flags(node))
{
  
}

shade_tree::sum::sum(const node_ptr &lhs, const node_ptr &rhs) :
  lhs(lhs), rhs(rhs),
  weight(get_weight(lhs) + get_weight(rhs)),
  flags(get_flags(lhs) | get_flags(rhs))
{
  
}

/* Helper class that evaluates a shade_tree */
class shade_tree_evaluator : public boost::static_visitor<> {
public:

  shade_tree::shader_flags mask;
  float3 *P_in, *w_in;
  float3 *P_out, *w_out;
  /* out */ float *pdf;
  /* out */ float4 *out;

  shade_tree_evaluator(shade_tree::shader_flags mask,
		       float3 *P_in, float3 *w_in,
		       float3 *P_out, float3 *w_out, 
		       /* out */ float *pdf, /* out */ float4 *out) :
    mask(mask),
    P_in(P_in), w_in(w_in), P_out(P_out), w_out(w_out), pdf(pdf), out(out)
  {
    
  }

  void operator()(shade_tree::leaf_ptr &node) const {
    shade_tree::shader_flags flags = node->get_flags();
    bool accepted = (mask == 0) || (flags & mask);
    if (accepted) node->evaluate(P_in, w_in, P_out, w_out, pdf, out);
    else {
      *pdf = 0.0f;
      *out = {0.0f, 0.0f, 0.0f, 0.0f};
    }
  }
  
  void operator()(shade_tree::scale_ptr &node) const {
    boost::apply_visitor(shade_tree_evaluator(mask, P_in, w_in, P_out, w_out, pdf, out), node->node);
    *out = node->k * (*out);
  }

  void operator()(shade_tree::sum_ptr &node) const {
    float4 lhs, rhs;
    shade_tree::shader_flags flags_L = get_flags(node->lhs);
    shade_tree::shader_flags flags_R = get_flags(node->rhs);

    bool accept_L = (mask == 0) || (flags_L & mask);
    bool accept_R = (mask == 0) || (flags_R & mask);
    
    float p_L = accept_L ? get_weight(mask, node->lhs) / get_weight(mask, node) : 0.0f;
    float p_R = accept_R ? 1.0f - p_L : 0.0f;
    float pdf0, pdf1;

    if (accept_L) boost::apply_visitor(shade_tree_evaluator(mask, P_in, w_in, P_out, w_out, &pdf0, &lhs), node->lhs);
    else {
      pdf0 = 0.0f;
      lhs = {0.0f, 0.0f, 0.0f, 0.0f};
    }

    if (accept_R) boost::apply_visitor(shade_tree_evaluator(mask, P_in, w_in, P_out, w_out, &pdf1, &rhs), node->rhs);
    else {
      pdf1 = 0.0f;
      rhs = {0.0f, 0.0f, 0.0f, 0.0f};
    }
    
    *out = lhs + rhs;
    *pdf = (p_L*pdf0 + p_R*pdf1);
  }

};

void shade_tree::evaluate(node_ptr &node, shade_tree::shader_flags mask,
			  float3 *P_in, float3 *w_in,
			  float3 *P_out, float3 *w_out,
			  /* out */ float *pdf, /* out */ float4 *out) {
  boost::apply_visitor(shade_tree_evaluator(mask, P_in, w_in, P_out, w_out, pdf, out), node);
}

/* PDF Calculation */

class pdf_visitor : public boost::static_visitor<float> {
public:

  shade_tree::shader_flags mask;
  float3 *P_in, *w_in;
  float3 *P_out, *w_out;

  pdf_visitor(shade_tree::shader_flags mask,
	      float3 *P_in, float3 *w_in,
	      float3 *P_out, float3 *w_out) :
    mask(mask),
    P_in(P_in), w_in(w_in), P_out(P_out), w_out(w_out)
  {
    
  }

  float operator()(shade_tree::leaf_ptr &node) const {
    return node->pdf(P_in, w_in, P_out, w_out);
  }

  float operator()(shade_tree::scale_ptr &node) const {
    bool accept = (mask == 0) || (get_flags(node->node) & mask);
    if (accept) return boost::apply_visitor(*this, node->node);
    return 0.0f;
  }

  float operator()(shade_tree::sum_ptr &node) const {
    shade_tree::shader_flags flags_L = get_flags(node->lhs);
    shade_tree::shader_flags flags_R = get_flags(node->rhs);

    bool accept_L = (mask == 0) || (flags_L & mask);
    bool accept_R = (mask == 0) || (flags_R & mask);
    
    float w_L = accept_L ? get_weight(mask, node->lhs) / get_weight(mask, node) : 0.0f;
    float w_R = accept_R ? 1.0f - w_L : 0.0f;
    
    float pdf_L = accept_L ? boost::apply_visitor(*this, node->lhs) : 0.0f;
    float pdf_R = accept_R ? boost::apply_visitor(*this, node->rhs) : 0.0f;

    return w_L*pdf_L + w_R*pdf_R;
  }

};

float shade_tree::pdf(node_ptr &node, shade_tree::shader_flags mask,
		      float3 *P_in, float3 *w_in,
		      float3 *P_out, float3 *w_out) {
  return boost::apply_visitor(pdf_visitor(mask, P_in, w_in, P_out, w_out), node);
}

/* Shade Tree Sampling */

class shade_tree_sampler : public boost::static_visitor<float> {

public:

  shade_tree::shader_flags mask;
  float3 *P_out, *w_out;
  float rand_D; //expected to be scaled by the tree's total weight
  float2 *rand_P, *rand_w;
  /* out */ float3 *P_in, *w_in;
  
  shade_tree_sampler(shade_tree::shader_flags mask,
		     float3 *P_out, float3 *w_out,
		     float rand_D, float2 *rand_P, float2 *rand_w,
		     /* out */ float3 *P_in, /* out */ float3 *w_in) :
    mask(mask),
    P_out(P_out), w_out(w_out),
    rand_D(rand_D), rand_P(rand_P), rand_w(rand_w),
    P_in(P_in), w_in(w_in)
  {
    
  }

  float operator()(shade_tree::leaf_ptr &node) const {
    return node->sample(P_out, w_out, rand_P, rand_w, P_in, w_in);
  }

  float operator()(shade_tree::scale_ptr &node) const {
    bool accept = (mask == 0) || (get_flags(node->node) & mask);
    if (accept) return boost::apply_visitor(*this, node->node);
    return 0.0f;
  }

  float operator()(shade_tree::sum_ptr &node) const {
    //select which argument to sample (based on weight)
    float total_weight = get_weight(mask, node);
    float sample_threshold = get_weight(mask, node->lhs);
    float w_L = sample_threshold / get_weight(mask, node);
    float w_R = 1.0 - w_L;
    float pdf_L, pdf_R;    

    if (rand_D < sample_threshold) {
      pdf_L = boost::apply_visitor(*this, node->lhs);
      pdf_R = boost::apply_visitor(pdf_visitor(mask, P_in, w_in, P_out, w_out), node->rhs);
    }
    else {
      pdf_L = boost::apply_visitor(pdf_visitor(mask, P_in, w_in, P_out, w_out), node->lhs);
      pdf_R = boost::apply_visitor(shade_tree_sampler(mask,
						      P_out, w_out,
						      rand_D - sample_threshold, rand_P, rand_w,
						      P_in, w_in),
				   node->rhs);
    }

    return w_L*pdf_L + w_R*pdf_R;
  }

};

float shade_tree::sample(node_ptr &node, shade_tree::shader_flags mask,
			 float3 *P_out, float3 *w_out,
			 float rand_D, float2 *rand_P, float2 *rand_w,
			 /* out */ float3 *P_in, /* out */ float3 *w_in) {
  float D = get_weight(mask, node) * rand_D;
  return boost::apply_visitor(shade_tree_sampler(mask, P_out, w_out, D, rand_P, rand_w, P_in, w_in),
			      node);						 
}

/* Emission */

class emission_visitor : public boost::static_visitor<void> {
public:

  shade_tree::shader_flags mask;
  float3 *P_out, *w_out;
  float4 *Le; //out

  emission_visitor(shade_tree::shader_flags mask,
		   float3 *P, float3 *w, float4 *Le) : mask(mask), P_out(P), w_out(w), Le(Le) { }

  void operator()(shade_tree::leaf_ptr &node) const {
    node->emission(P_out, w_out, Le);
  }
  
  void operator()(shade_tree::scale_ptr &node) const {
    bool accept = (mask == 0) || (get_flags(node->node) & mask);
    if (accept) {
      boost::apply_visitor(*this, node->node);
      *Le = node->k * (*Le);
    }
    else *Le = {0.0f, 0.0f, 0.0f, 0.0f};
  }

  void operator()(shade_tree::sum_ptr &node) const {
      bool accept_L = (mask == 0) || (get_flags(node->lhs) & mask);
      bool accept_R = (mask == 0) || (get_flags(node->rhs) & mask);

    float4 Le0{0.0f, 0.0f, 0.0f, 0.0f};
    float4 Le1{0.0f, 0.0f, 0.0f, 0.0f};
    
    if (accept_L) boost::apply_visitor(emission_visitor(mask, P_out, w_out, &Le0), node->lhs);
    if (accept_R) boost::apply_visitor(emission_visitor(mask, P_out, w_out, &Le1), node->rhs);
    *Le = Le0 + Le1;
  }
  
};

void shade_tree::emission(node_ptr &node, shade_tree::shader_flags mask,
			  float3 *P_out, float3 *w_out,
			  /* out */ float4 *Le) {
  boost::apply_visitor(emission_visitor(mask, P_out, w_out, Le),
		       node);		 
}
