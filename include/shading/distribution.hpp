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
      typedef void (*eval_func_type)(const void*,
				     float4*,
				     float3*, float3*, float3*, float3*,
				     float4*);

      typedef void (*dtor_func_type)(void*);

      leaf(char *p, eval_func_type eval, dtor_func_type dtor);
      ~leaf();
      
      char *params;
      eval_func_type evaluate;
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
    
    struct scale {
      float4 k;
      node_ptr node;
    };
    
    struct sum {
      node_ptr lhs, rhs;
    };

    void evaluate(node_ptr &node,
		  float4 *L_in,
		  float3 *P_in, float3 *w_in,
		  float3 *P_out, float3 *w_out,
		  /* out */ float4 *out);
  };
};

#endif
