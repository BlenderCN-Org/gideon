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
