#ifndef RT_RANDOM_HPP
#define RT_RANDOM_HPP

#include <random>
#include <functional>

namespace raytrace {

  /* Class that generates a uniform random number in [0, 1). */
  class random_number_gen {
  public:

    random_number_gen();

    float operator()() const;

  private:

    std::function<float ()> gen;

  };

};

#endif
