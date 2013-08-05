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

#ifndef GD_RENDER_CONTEXT_HPP
#define GD_RENDER_CONTEXT_HPP

#include "scene/scene.hpp"
#include "scene/bvh.hpp"
#include "math/sampling.hpp"

#include "compiler/rendermodule.hpp"

#include <boost/function.hpp>
//#include <OpenImageIO/texture.h>

namespace gideon {

  /* Contains data relevant to the current rendering session (scene, bvh, programs, etc). */
  class render_context {
  public:

    struct scene_data {
      raytrace::scene *s;
      raytrace::bvh *accel;
      raytrace::sampler samples;
      boost::function<float ()> rng; //probably not threadsafe

      //OpenImageIO::TextureSystem *textures;
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
