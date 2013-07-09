/*

  Copyright 2013 Curtis Andrus

  This file is part of Gideon.

  Gideon is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  Gideon is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with Gideon.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "scene/bvh.hpp"
#include <iostream>

using namespace std;
using namespace raytrace;

__thread int raytrace::bvh::traversal_stack[raytrace::bvh::max_stack_depth];

raytrace::bvh::bvh(const scene &s,
		   const vector<bvh::node> &node_list,
		   const vector<int> &leaf_prim_list) :
  active_scene(&s),
  num_nodes(node_list.size()),
  nodes(new node[node_list.size()]),
  leaf_array(new int[leaf_prim_list.size()])
{
  copy(node_list.begin(), node_list.end(), nodes);
  copy(leaf_prim_list.begin(), leaf_prim_list.end(), leaf_array);
}

raytrace::bvh::~bvh() {
  delete[] nodes;
  delete[] leaf_array;
}

void raytrace::bvh::debug_print() const {
  for (unsigned int i = 0; i < num_nodes; i++) {
    cout << "Node " << i << ": ";
    cout << "Type: " << (nodes[i].type == node::INNER ? "INNER" : "LEAF") << ", ";
    
    if (nodes[i].type == node::INNER) {
      cout << "Children: (" << nodes[i].indices.x << ", " << nodes[i].indices.y << ")";
    }
    else {
      cout << "Num Primitives: " << nodes[i].indices.y - nodes[i].indices.x << " | Range: ["
	   << nodes[i].indices.x << ", " << nodes[i].indices.y << "]";
    }

    cout << endl;

    const aabb &bounds = nodes[i].bounds;
    cout << "Node " << i << " Bounds: [" << bounds.pmin.x << ", " << bounds.pmax.x << "] x ["
	 << bounds.pmin.y << ", " << bounds.pmax.y << "] x ["
	 << bounds.pmin.z << ", " << bounds.pmax.z << "]" << endl;
  }
}

bool raytrace::bvh::trace(const ray &r,
			  /* out */ intersection &isect,
			  /* out */ unsigned int &aabb_checked,
			  /* out */ unsigned int &prim_checked) const {
  aabb_checked = 0;
  prim_checked = 0;
  float closest_t = r.max_t;

  if (num_nodes == 0) return false;

  //first check the root node
  node &root = nodes[0];
  aabb_checked++;

  float t0, t1;
  if (!ray_aabb_intersection(root.bounds, r, t0, t1)) return false;
  
  //if the root is also a leaf, check its primitives and return
  if (root.type == node::LEAF) return intersect_leaf(root, r, closest_t, isect, prim_checked);

  //traverse the hierarchy
  bool hit_prim = false;
  
  traversal_stack[0] = 0; //start at the root node
  size_t stack_size = 1;

  while (stack_size > 0) {
    //pop the next node off the stack
    int node_idx = traversal_stack[stack_size-1];
    stack_size--;

    node &curr_node = nodes[node_idx];
    //cout << "Checking Node: " << node_idx << endl;
    
    if (curr_node.type == node::LEAF) continue; //we've already checked leaf nodes
    
    //ray-test both children
    node &left_child = nodes[curr_node.indices.x];
    node &right_child = nodes[curr_node.indices.y];

    float2 left_range, right_range;
    bool check_left = ray_aabb_intersection(left_child.bounds, r, left_range.x, left_range.y);
    bool check_right = ray_aabb_intersection(right_child.bounds, r, right_range.x, right_range.y);
    aabb_checked += 2;
    
    //cout << "Checking Left: " << check_left << endl;
    //cout << "Checking Right: " << check_right << endl;

    //only check a node if it's closer than the current closest hit
    if (hit_prim) {
      check_left = (check_left && (left_range.x < closest_t));
      check_right = (check_right && (right_range.x < closest_t));
    }
    
    if (!check_left && !check_right) continue; //no possible hits, move on
    
    if (!check_left) {
      //check only the right child
      check_node(right_child, curr_node.indices.y, r, closest_t, isect, prim_checked, hit_prim, stack_size);
    }
    else if (!check_right) {
      //check only the left child
      check_node(left_child, curr_node.indices.x, r, closest_t, isect, prim_checked, hit_prim, stack_size);
    }
    else {
      //check the closest child first
      if (left_range.x < right_range.x) {
	check_node(left_child, curr_node.indices.x, r, closest_t, isect, prim_checked, hit_prim, stack_size);
	if (!hit_prim || (right_range.x < closest_t)) check_node(right_child, curr_node.indices.y, r, closest_t, isect, prim_checked, hit_prim, stack_size);
      }
      else {
	check_node(right_child, curr_node.indices.y, r, closest_t, isect, prim_checked, hit_prim, stack_size);
	if (!hit_prim || (left_range.x < closest_t)) check_node(left_child, curr_node.indices.x, r, closest_t, isect, prim_checked, hit_prim, stack_size);
      }
    }

    //cout << "- - - - -" << endl;
  }

  return hit_prim;
}

void raytrace::bvh::check_node(const node &n, int n_idx,
			       const ray &r,
			       /* inout */ float &closest_t, /* inout */ intersection &isect,
			       /* inout */ unsigned int &prim_checked,
			       /* inout */ bool &hit_prim,
			       /* inout */ size_t &stack_size) const {
  if (n.type == node::LEAF) {
    bool hit = intersect_leaf(n, r, closest_t, isect, prim_checked);
    hit_prim = hit || hit_prim;
  }
  else {
    traversal_stack[stack_size++] = n_idx;
  }
}

bool raytrace::bvh::intersect_leaf(const node &leaf,
				   const ray &r,
				   /* inout */ float &closest_t, /* out */ intersection &isect,
				   /* inout */ unsigned int &prim_checked) const {
  bool found_hit = false;
  intersection tmp;
  
  for (int i = leaf.indices.x; i < leaf.indices.y; i++) {
    int prim_idx = leaf_array[i];
    const primitive &prim = active_scene->primitives[prim_idx];
    if (ray_primitive_intersection(prim, *active_scene, r, tmp)) {
      if (tmp.t < closest_t) {
	tmp.prim_idx = prim_idx;
	isect = tmp;
	closest_t = tmp.t;
	found_hit = true;
      }
    }
  }

  prim_checked += (leaf.indices.y - leaf.indices.x);
  return found_hit;
}
