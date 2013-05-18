#ifndef GD_RENDER_CONTEXT_HPP
#define GD_RENDER_CONTEXT_HPP

#include "scene/scene.hpp"
#include "scene/bvh.hpp"
#include "compiler/rendermodule.hpp"

#include <boost/function.hpp>

namespace gideon {

  /* Contains data relevant to the current rendering session (scene, bvh, programs, etc). */
  class render_context {
  public:

    struct scene_data {
      raytrace::scene *s;
      raytrace::bvh *accel;
      boost::function<float ()> rng; //probably not threadsafe
    };

    render_context();
    ~render_context();

    //Sets the context's render kernel (takes ownership of the passed object).
    void set_kernel(std::unique_ptr<raytrace::render_kernel> k);
    
    //Returns a pointer to the current kernel (for use in function lookups).
    raytrace::render_kernel *get_kernel() { return kernel.get(); }
    
    //Sets the context's current scene.
    void set_scene(std::unique_ptr<raytrace::scene> s);

    //Rebuild's the scene's BVH.
    void build_bvh();

  private:

    std::unique_ptr<raytrace::scene> scn;
    std::unique_ptr<raytrace::render_kernel> kernel;
    std::unique_ptr<raytrace::bvh> accel;

    scene_data *sd;
    
  };

};

#endif
