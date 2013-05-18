#ifndef RT_OBJECT_HPP
#define RT_OBJECT_HPP

#include <map>
#include <vector>
#include <string>

#include "math/vector.hpp"
#include "scene/attribute.hpp"

#include <memory>

namespace raytrace {

  /* Container for per-object attributes. */
  struct object {
    ~object();
    
    int2 vert_range, prim_range, tri_range;
    std::map<std::string, attribute*> attributes;
  };

  typedef std::shared_ptr<object> object_ptr;
  
};

#endif
