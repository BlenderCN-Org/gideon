#include "scene/scene.hpp"

using namespace std;
using namespace raytrace;

void raytrace::scene::clear() {
  vertices.clear();
  vertex_normals.clear();
  triangle_verts.clear();

  primitives.clear();
  objects.clear();

  lights.clear();
}
