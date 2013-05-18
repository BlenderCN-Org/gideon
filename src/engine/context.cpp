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
