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
