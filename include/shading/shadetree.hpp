#ifndef RT_SHADE_TREE_HPP
#define RT_SHADE_TREE_HPP

#include "math/vector.hpp"

#include <memory>
#include <vector>

namespace raytrace {

  class vm;
  class distribution_instance;
  
  struct shader_node {
    typedef enum { DISTRIBUTION, SUM, PRODUCT, SCALE } node_type;

    node_type type;
    std::shared_ptr<distribution_instance> func;
    std::shared_ptr<shader_node> lhs, rhs;
    float4 k;

    shader_node(const std::shared_ptr<distribution_instance> &f) :
      type(DISTRIBUTION), func(f),
      lhs(nullptr), rhs(nullptr),
      k({0.0f, 0.0f, 0.0f, 0.0f}) {}

    shader_node(const node_type t,
		const std::shared_ptr<shader_node> &lhs, const std::shared_ptr<shader_node> &rhs) :
      type(t), func(nullptr),
      lhs(lhs), rhs(rhs),
      k({0.0f, 0.0f, 0.0f, 0.0f}) {}

    shader_node(const float4 &k,
		const std::shared_ptr<shader_node> &op) :
      type(SCALE), func(nullptr),
      lhs(nullptr), rhs(op),
      k(k) {}
  };
  
  //Returns the value of the shade formula at the given point.
  float4 evaluate_shade_formula(vm *svm, shader_node &n,
				const float3 &N,
				const float3 &P_in, const float3 &P_out,
				const float3 &w_in, const float3 &w_out);
  
};

#endif
