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

#include "engine/context.hpp"
#include "scene/bvh_builder.hpp"

using namespace std;
using namespace gideon;

render_context::render_context() :
  sd(new scene_data)
{
  sd->rng = bind(uniform_real_distribution<float>(0.0f, 1.0f),
		 mt19937());
}

render_context::~render_context() {
  if (sd) delete sd;
}

void render_context::set_kernel(unique_ptr<raytrace::render_kernel> k) {
  kernel = move(k);
  
  //map the kernel's global scene variable to this context
  kernel->map_global(".__gd_scene", reinterpret_cast<void*>(&sd));
}

void render_context::set_scene(unique_ptr<raytrace::scene> s) {
  scn = move(s);
  sd->s = scn.get();
}

void render_context::build_bvh() {
  accel.reset(new raytrace::bvh(raytrace::build_bvh_centroid_sah(scn.get())));
  sd->accel = accel.get();
}
