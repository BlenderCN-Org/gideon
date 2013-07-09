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

#ifndef GD_STD_OPS_HPP
#define GD_STD_OPS_HPP

#include "scene/scene.hpp"
#include "scene/bvh.hpp"
#include "geometry/ray.hpp"

#include <boost/function.hpp>

namespace gideon {

  namespace rl {
    
    //Data that the built-in functions expect to be associated with the scene pointer.
    struct scene_data {
      raytrace::scene *s;
      raytrace::bvh *accel;
      boost::function<float ()> rng;

      scene_data(raytrace::scene *s, raytrace::bvh *accel);
    };

  };
  
};

#endif
