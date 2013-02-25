#include "math/random.hpp"

using namespace std;
using namespace raytrace;

raytrace::random_number_gen::random_number_gen() :
  gen(bind(uniform_real_distribution<float>(0.0f, 1.0f), mt19937()))
{
  
}

float raytrace::random_number_gen::operator()() const {
  return gen();
}
