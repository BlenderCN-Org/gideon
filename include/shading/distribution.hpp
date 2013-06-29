#ifndef RT_DISTRIBUTION_HPP
#define RT_DISTRIBUTION_HPP

#include "math/vector.hpp"

#include <boost/unordered_map.hpp>
#include <boost/variant.hpp>

#include <string>
#include <map>

namespace raytrace {
  
  namespace shade_tree {
    struct leaf {
    public:
      
      typedef void (*eval_func_type)(const void*,
				     float3*, float3*, float3*, float3*,
				     float*, float4*);
      typedef float (*sample_func_type)(const void *,
					float3*, float3*,
					float2*, float2*,
					float3*, float3*);
      typedef float (*pdf_func_type)(const void *,
				     float3*, float3*, float3*, float3*);

      typedef void (*emission_func_type)(const void*,
					 float3*, float3*, float4*);

      typedef void (*dtor_func_type)(void*);

      leaf(char *p,
	   eval_func_type eval, sample_func_type sample,
	   pdf_func_type pdf, emission_func_type emit,
	   dtor_func_type dtor);
      ~leaf();
      
      void evaluate(float3 *P_in, float3 *w_in,
		    float3 *P_out, float3 *w_out,
		    /* out */ float *pdf, /* out */ float4 *eval) const;

      float pdf(float3 *P_in, float3 *w_in,
		float3 *P_out, float3 *w_out) const;

      float sample(float3 *P_out, float3 *w_out,
		   float2 *rand_P, float2 *rand_w,
		   /* out */ float3 *P_in, /* out */ float3 *w_in) const;

      void emission(float3 *P_out, float3 *w_out, /* out */ float4 *Le) const;

    private: 
      
      char *params;
      eval_func_type evaluate_fn;
      sample_func_type sample_fn;
      pdf_func_type pdf_fn;
      emission_func_type emit_fn;

      dtor_func_type destructor;
    };

    struct scale;
    struct sum;
    
    typedef std::shared_ptr<leaf> leaf_ptr;
    typedef std::shared_ptr<scale> scale_ptr;
    typedef std::shared_ptr<sum> sum_ptr;

    typedef boost::variant<leaf_ptr,
			   scale_ptr,
			   sum_ptr> node_ptr;
    
    float get_weight(const node_ptr &node);
    
    struct scale {
      float4 k;
      node_ptr node;
      float weight;

      scale(const float4 &k, const node_ptr &node);
    };
    
    struct sum {
      node_ptr lhs, rhs;
      float weight;

      sum(const node_ptr &lhs, const node_ptr &rhs);
    };

    void evaluate(node_ptr &node,
		  float3 *P_in, float3 *w_in,
		  float3 *P_out, float3 *w_out,
		  /* out */ float *pdf, /* out */ float4 *out);

    float sample(node_ptr &node,
		 float3 *P_out, float3 *w_out,
		 float rand_D, float2 *rand_P, float2 *rand_w,
		 /* out */ float3 *P_in, /* out */ float3 *w_in);

    void emission(node_ptr &node,
		  float3 *P_out, float3 *w_out,
		  /* out */ float4 *Le);
    
  };
};

#endif
