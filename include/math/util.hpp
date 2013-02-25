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
