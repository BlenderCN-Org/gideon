#include "scene/scene.hpp"

using namespace std;
using namespace raytrace;

raytrace::scene::~scene() {
  for (vector<object*>::iterator it = objects.begin(); it != objects.end(); it++)
    delete *it;
}
