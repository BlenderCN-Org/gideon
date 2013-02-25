#include "scene/bvh_builder.hpp"
#include "scene/bvh.hpp"

#include <algorithm>
#include <stack>
#include <iostream>

using namespace std;
using namespace raytrace;

static const float T_tri = 1.0f;
//static const float T_aabb = 1.5f;
static const float T_aabb = 3.0f;

bvh raytrace::build_bvh_centroid_sah(const scene *active_scene) {
  vector<bvh::node> node_list;
  node_list.push_back(bvh::node());
  
  vector<int> master_prim_list;

  stack<int> partition_stack;
  stack<int2> primitive_stack;
  
  vector<int> prim_list;
  prim_list.reserve(active_scene->primitives.size());
  for (unsigned int i = 0; i < active_scene->primitives.size(); i++) prim_list.push_back(static_cast<int>(i));
  
  //precompute the centroids
  vector<float3> centroids;
  centroids.reserve(active_scene->primitives.size());
  for (unsigned int i = 0; i < active_scene->primitives.size(); i++) {
    centroids.push_back(primitive_bbox(active_scene->primitives[i], *active_scene).center());
  }

  //start with the root node
  partition_stack.push(0); 
  primitive_stack.push({0, static_cast<int>(prim_list.size())});
  
  //cout << "Splitting Nodes..." << endl;

  //recursively split nodes as needed
  while (partition_stack.size() > 0) {
    int node_idx = partition_stack.top();
    int2 primitives = primitive_stack.top();

    partition_stack.pop();
    primitive_stack.pop();

    int2 left_prims, right_prims;
    bool do_split = centroid_sah_partition_node(active_scene, centroids, 
						prim_list, primitives,
						left_prims, right_prims);

    bvh::node &node = node_list[node_idx];
    node.type = (do_split ? bvh::node::INNER : bvh::node::LEAF);
    //we'll compute the bounding box later
    
    if (do_split) {
      //add the child nodes to the stack
      //cout << "Splitting Node " << node_idx << endl;
      int children_start = static_cast<int>(node_list.size());
      node.indices = int2{children_start, children_start+1};

      node_list.push_back(bvh::node());
      partition_stack.push(children_start);
      primitive_stack.push(left_prims);

      node_list.push_back(bvh::node());
      partition_stack.push(children_start+1);
      primitive_stack.push(right_prims);
    }
    else {
      //add the primitives to the final list
      //cout << "Leaf Node: " << node_idx << endl;
      int num_prims = primitives.y - primitives.x;
      int prim_start = static_cast<int>(master_prim_list.size());
      int prim_end = prim_start + num_prims;

      node.indices = int2{prim_start, prim_end};
      for (int i = primitives.x; i < primitives.y; i++) master_prim_list.push_back(prim_list[i]);
    }
  }

  //cout << "...Done. Computing Bounding Boxes." << endl;

  //traverse all nodes, building bounding boxes starting from leaves
  partition_stack.push(0);

  vector<int> node_history(node_list.size(), 0);

  while (partition_stack.size() > 0) {
    int node_idx = partition_stack.top();
    partition_stack.pop();

    bvh::node &node = node_list[node_idx];

    if (node.type == bvh::node::LEAF) node.bounds = primitive_list_bounds(active_scene, master_prim_list, node.indices);
    else if (node_history[node_idx] == 1) {
      //we've seen this node before, so we must know its children's bounds
      bvh::node &left = node_list[node.indices.x];
      bvh::node &right = node_list[node.indices.y];
      node.bounds = left.bounds.merge(right.bounds);
    }
    else {
      //new inner node, check if the children are leaves, otherwise check it later
      node_history[node_idx] = 1;
      bvh::node &left = node_list[node.indices.x];
      bvh::node &right = node_list[node.indices.y];
      
      partition_stack.push(node_idx);
      partition_stack.push(node.indices.x);
      partition_stack.push(node.indices.y);
    }
  }

  //cout << "Done." << endl;
  
  cout << "Num Primitives: " << master_prim_list.size() << " | Should Be: " << active_scene->primitives.size() << endl;
  return bvh(*active_scene, node_list, master_prim_list);
}

struct centroid_prim_cmp {
  const vector<float3> &prim_centroids;
  int axis;

  bool operator()(const int a, const int b) {
    return prim_centroids[a][axis] < prim_centroids[b][axis];
  }
};

float raytrace::partition_surface_area(const scene *active_scene, const vector<float3> &centroids,
				       const vector<int> &primitives,
				       int p_start, int p_end) {
  if (p_start == p_end) return numeric_limits<float>::max();

  aabb bbox = aabb::empty_box();

  for (int i = p_start; i < p_end; i++) {
    const float3 &c = centroids[primitives[i]];
    if (c.x < bbox.pmin.x) bbox.pmin.x = c.x;
    if (c.x > bbox.pmax.x) bbox.pmax.x = c.x;

    if (c.y < bbox.pmin.y) bbox.pmin.y = c.y;
    if (c.y > bbox.pmax.y) bbox.pmax.y = c.y;

    if (c.z < bbox.pmin.z) bbox.pmin.z = c.z;
    if (c.z > bbox.pmax.z) bbox.pmax.z = c.z;

    //bbox = bbox.merge(primitive_bbox(active_scene->primitives[primitives[i]], *active_scene));
  }

  return bbox.surfacearea();
}

float update_sah(const vector<int> &primitives, const vector<float3> &centroids, int idx,
		 /* inout */ aabb &bbox) {
  const float3 &c = centroids[primitives[idx]];
  if (c.x < bbox.pmin.x) bbox.pmin.x = c.x;
  if (c.x > bbox.pmax.x) bbox.pmax.x = c.x;
  
  if (c.y < bbox.pmin.y) bbox.pmin.y = c.y;
  if (c.y > bbox.pmax.y) bbox.pmax.y = c.y;
  
  if (c.z < bbox.pmin.z) bbox.pmin.z = c.z;
  if (c.z > bbox.pmax.z) bbox.pmax.z = c.z;

  return bbox.surfacearea();
}
		 

aabb raytrace::primitive_list_bounds(const scene *active_scene, const std::vector<int> &primitives, const int2 &range) {
  aabb bbox = aabb::empty_box();
  
  for (int i = range.x; i < range.y; i++) {
    bbox = bbox.merge(primitive_bbox(active_scene->primitives[primitives[i]], *active_scene));
  }

  return bbox;
}

int raytrace::centroid_sah_find_best_partition_axis_event(const scene *active_scene, const vector<float3> &centroids,
							  vector<int> &primitives, const int2 &range,
							  int axis, /* out */ float &best_cost) {

  
  //sort primitives by centroid on the given axis
  //cout << "Sorting [" << range.x << ", " << range.y << "] on Axis " << axis << " | " << range.y - range.x << endl;
  centroid_prim_cmp cmp{centroids, axis};
  sort(primitives.begin() + range.x, primitives.begin() + range.y, cmp);
  //cout << "Sort Complete" << endl;

  int num_primitives = range.y - range.x;
  float total_surface_area = partition_surface_area(active_scene, centroids, primitives, range.x, range.y);
 
  best_cost = num_primitives * T_tri;
  int best_event = -1;

  vector<float> right_areas(num_primitives, 0.0f);
  
  //sweep right to left
  aabb right_box = aabb::empty_box();
  for (int i = (range.y - 1); i >= range.x; i--) {
    int offset = i - range.x;
    right_areas[offset] = update_sah(primitives, centroids, i, right_box);
  }

  //for each possible partitioning event
  aabb left_box = aabb::empty_box();
  float left_area = numeric_limits<float>::max();

  for (int i = range.x; i < range.y; i++) {
    //Compute area of shapes on left and right
    int offset = i - range.x;
    //float left_area = partition_surface_area(active_scene, centroids, primitives, range.x, i);
    //float right_area = partition_surface_area(active_scene, centroids, primitives, i, range.y);
    float right_area = right_areas[offset];
    
    //Evaluate cost function
    float s1_cost = (left_area / total_surface_area) * offset;
    float s2_cost = (right_area / total_surface_area) * (num_primitives - offset);
    float T = 2.0f*T_aabb + s1_cost*T_tri + s2_cost*T_tri;
    
    if (T < best_cost) {
      best_event = i;
      best_cost = T;
    }

    left_area = update_sah(primitives, centroids, i, left_box);
  }

  //cout << "Best Event: " << best_event << " | Cost: " << best_cost << endl;
  //cout << "Axis " << axis << " complete." << endl;

  return best_event;
}

int raytrace::centroid_sah_best_partition(const scene *active_scene, const vector<float3> &centroids,
					  vector<int> &primitives, const int2 &range,
					  /* out */ int &best_axis, /* out */ float &best_cost) {
  int num_primitives = range.y - range.x;
  
  best_axis = -1;
  best_cost = num_primitives * T_tri;
  int best_event = -1;
 
  for (int axis = 0; axis < 3; axis++) {
    float cost = 0.0f;
    int event = centroid_sah_find_best_partition_axis_event(active_scene, centroids, primitives, range, axis, cost);

    if (cost < best_cost) {
      best_cost = cost;
      best_event = event;
      best_axis = axis;
    }
  }

  return best_event;
}

bool raytrace::centroid_sah_partition_node(const scene *active_scene, const vector<float3> &centroids,
					   vector<int> &node_primitives, const int2 &range,
					   /* out */ int2 &left_prims, /* out */ int2 &right_prims) {
  float best_cost;
  int best_axis;
  int best_event = centroid_sah_best_partition(active_scene, centroids, node_primitives, range,
					       best_axis, best_cost);
  
  if (best_event == -1) return false; //make leaf
  
  //sort and partition the list
  centroid_prim_cmp cmp{centroids, best_axis};
  sort(node_primitives.begin() + range.x, node_primitives.begin() + range.y, cmp);
  
  left_prims = int2{range.x, best_event};
  right_prims = int2{best_event, range.y};
  return true;
}
