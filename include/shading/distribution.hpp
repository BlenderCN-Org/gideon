#ifndef RT_DISTRIBUTION_HPP
#define RT_DISTRIBUTION_HPP

#include "math/vector.hpp"
#include "vm/parameters.hpp"

#include <boost/unordered_map.hpp>
#include <boost/variant.hpp>

#include <string>
#include <map>

namespace raytrace {

  class vm;
  class program;

  class scene;
  
  /*
    A program that describes how light intersects with a surface/volume.
  */
  class distribution_function {
  public:

    distribution_function(const std::string &name, int flags,
			  program *p, unsigned int eval, unsigned int sample,
			  const std::map<std::string, size_t> &params);
    
    //Returns the value of this function at the given point.
    float4 evaluate(vm *svm, parameter_list *params,
		    const float3 &N,
		    const float3 &P_in, const float3 &P_out,
		    const float3 &w_in, const float3 &w_out);

    //Samples a position & direction from this distribution.
    //Returns the probability of the result.
    float sample(vm *svm, parameter_list *params,
		 const float3 &N, const float3 &P_in, const float3 &w_in,
		 /* out */ float3 &P_out, /* out */ float3 &w_out,
		 /* out */ float4 &eval);

    //Returns the amount of bytes needed for this functions parameters.
    size_t param_size() const;

    //Bitmask of the various flags supported by this distribution.
    int flags;
    
    /* List of Distribution Flags */
    
    const static int CONSTANT = 1;

    const static int DIFFUSE = 2;
    const static int SPECULAR = 4;
    const static int TRANSPARENT = 8;
    const static int EMISSIVE = 16;

    const static int SUBSURFACE = 32;
    const static int VOLUME = 64;
    
  private:
    
    std::string name;
    program *prog;
    unsigned int eval_entry, sample_entry;
    
    std::map<std::string, size_t> plist;
  };


  /*
    A distribution function bound to a set of parameters.
  */
  struct distribution_instance {
    
    distribution_instance(scene *s, int func_id);

    int id;
    parameter_list params;
    
  };
  
  namespace shade_tree {
    struct leaf {
      typedef void (*eval_func_type)(const void*,
				     float3*,
				     float3*, float3*, float3*, float3*,
				     float4*);

      ~leaf() { delete[] params; }
      
      char *params;
      eval_func_type evaluate;
    };

    struct scale;
    struct sum;
    
    typedef boost::variant<leaf,
			   scale,
			   sum> node;
    typedef std::shared_ptr<node> node_ptr;
    
    struct scale {
      float4 k;
      node_ptr node;
    };
    
    struct sum {
      node_ptr lhs, rhs;
    };

    void evaluate(node_ptr &node,
		  float3 *N,
		  float3 *P_in, float3 *w_in,
		  float3 *P_out, float3 *w_out,
		  /* out */ float4 *out);
  };
};

#endif
