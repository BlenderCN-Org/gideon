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

#ifndef RT_BVH_BUILDER_HPP
#define RT_BVH_BUILDER_HPP

#include "math/vector.hpp"
#include <vector>

namespace raytrace {

  class scene;
  class bvh;
  class aabb;

  bvh build_bvh_centroid_sah(const scene *active_scene);

  /* Centroid SAH Helper Functions */
  int centroid_sah_find_best_partition_axis_event(const scene *active_scene, const std::vector<float3> &centroids,
						  std::vector<int> &primitives, const int2 &range,
						  int axis, /* out */ float &best_cost);

  int centroid_sah_best_partition(const scene *active_scene, const std::vector<float3> &centroids,
				  std::vector<int> &primitives, const int2 &range,
				  /* out */ int &best_axis, /* out */ float &best_cost);

  float partition_surface_area(const scene *active_scene, const std::vector<float3> &centroids,
			       const std::vector<int> &primitives,
			       int p_start, int p_end);

  aabb primitive_list_bounds(const scene *active_scene, const std::vector<int> &primitives, const int2 &range);

  bool centroid_sah_partition_node(const scene *active_scene, const std::vector<float3> &centroids,
				   std::vector<int> &node_primitives, const int2 &range,
				   /* out */ int2 &left_prims, /* out */ int2 &right_prims);
};

#endif
