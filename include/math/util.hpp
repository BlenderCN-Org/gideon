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

#ifndef RT_MATH_UTIL_HPP
#define RT_MATH_UTIL_HPP

namespace raytrace {

  template<typename T>
  void minmax3(const T &x, const T &y, const T &z,
	       /*out */ T &min_val, /* out */ T &max_val) {
    min_val = max_val = x;
    if (y < min_val) min_val = y;
    else if (y > max_val) max_val = y;

    if (z < min_val) min_val = z;
    else if (z > max_val) max_val = z;
  }
  
  template<typename T>
  void minmax2(const T &x, const T &y,
	       /* out */ T &min_val, /* out */ T &max_val) {
    if (x < y) {
      min_val = x;
      max_val = y;
    }
    else {
      min_val = y;
      max_val = x;
    }
  }
};

#endif
