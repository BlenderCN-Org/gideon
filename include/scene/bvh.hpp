#ifndef RT_BVH_HPP
#define RT_BVH_HPP

#include "geometry/ray.hpp"
#include "geometry/aabb.hpp"
#include "scene/scene.hpp"

#include <vector>

namespace raytrace {  

  class bvh {
  public:
    
    //describes a single node in the bvh tree
    struct node {
      enum { INNER, LEAF } type;
      aabb bounds;
      int2 indices; //children in case of inner node, range of primitives in case of leaf
    };

    bvh(const scene &s,
	const std::vector<node> &node_list,
	const std::vector<int> &leaf_prim_list);
    
    ~bvh();
    
    bool trace(const ray &r,
	       /* out */ intersection &isect,
	       /* out */ unsigned int &aabb_checked,
	       /* out */ unsigned int &prim_checked) const;

    void debug_print() const;
    
  private:

    const scene *active_scene; //for accessing primitives

    unsigned int num_nodes;
    node *nodes; //array of nodes, root node is at 0
    int *leaf_array; //array containing the contents of each leaf
    
    void check_node(const node &n, int n_idx,
		    const ray &r,
		    /* inout */ float &closest_t, /* inout */ intersection &isect,
		    /* inout */ unsigned int &prim_checked,
		    /* inout */ bool &hit_prim,
		    /* inout */ size_t &stack_size) const;

    bool intersect_leaf(const node &leaf,
			const ray &r,
			/* inout */ float &closest_t, /* out */ intersection &isect,
			/* inout */ unsigned int &prim_checked) const;
    
    //use thread-local traversal stack so we can use this bvh in multiple threads
    static const size_t max_stack_depth = 256;
    static __thread int traversal_stack[];
    
  };
};

#endif
