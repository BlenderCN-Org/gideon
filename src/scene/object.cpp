#include "scene/object.hpp"

using namespace std;
using namespace raytrace;

raytrace::object::~object() {
  for (map<string, attribute*>::iterator it = attributes.begin(); it != attributes.end(); it++) {
    delete it->second;
  }
}
