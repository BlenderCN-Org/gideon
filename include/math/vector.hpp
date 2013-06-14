#ifndef RT_VECTOR_HPP
#define RT_VECTOR_HPP

namespace raytrace {

  const float epsilon = 0.00001f;

  /* Vector Generic Types */

  template<typename T>
  struct gen_vec2 {
    T x, y;
    
    inline T operator[](int i) const { return *(&x + i); }
    inline T &operator[](int i) { return *(&x + i); }
  };

  template<typename T>
  struct gen_vec3 {
    T x, y, z;
    
    inline T operator[](int i) const { return *(&x + i); }
    inline T &operator[](int i) { return *(&x + i); }
  };

  template<typename T>
  struct gen_vec4 {
    T x, y, z, w;
    
    inline T operator[](int i) const { return *(&x + i); }
    inline T &operator[](int i) { return *(&x + i); }
  };

  /* Usable Vector Types */

  typedef gen_vec2<float> float2;
  typedef gen_vec3<float> float3;
  typedef gen_vec4<float> float4;

  typedef gen_vec2<int> int2;
  typedef gen_vec3<int> int3;
  typedef gen_vec4<int> int4;
  
  /* Vector Arithmetic */
  
  //Addition
  template<typename T>
  gen_vec2<T> operator+(const gen_vec2<T> &a, const gen_vec2<T> &b) { return {a.x + b.x, a.y + b.y}; }

  template<typename T>
  gen_vec3<T> operator+(const gen_vec3<T> &a, const gen_vec3<T> &b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }

  template<typename T>
  gen_vec4<T> operator+(const gen_vec4<T> &a, const gen_vec4<T> &b) { return {a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w}; }

  //Subtraction
  template<typename T>
  gen_vec2<T> operator-(const gen_vec2<T> &a, const gen_vec2<T> &b) { return {a.x - b.x, a.y - b.y}; }

  template<typename T>
  gen_vec3<T> operator-(const gen_vec3<T> &a, const gen_vec3<T> &b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }

  template<typename T>
  gen_vec4<T> operator-(const gen_vec4<T> &a, const gen_vec4<T> &b) { return {a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w}; }

  //Scalar Multiplication
  template<typename T>
  gen_vec2<T> operator*(const T &k, const gen_vec2<T> &v) { return {k*v.x, k*v.y}; }

  template<typename T>
  gen_vec3<T> operator*(const T &k, const gen_vec3<T> &v) { return {k*v.x, k*v.y, k*v.z}; }

  template<typename T>
  gen_vec4<T> operator*(const T &k, const gen_vec4<T> &v) { return {k*v.x, k*v.y, k*v.z, k*v.w}; }

  //Scalar Division
  template<typename T>
  gen_vec2<T> operator/(const gen_vec2<T> &v, const T &k) { return {v.x/k, v.y/k}; }

  template<typename T>
  gen_vec3<T> operator/(const gen_vec3<T> &v, const T &k) { return {v.x/k, v.y/k, v.z/k}; }

  template<typename T>
  gen_vec4<T> operator/(const gen_vec4<T> &v, const T &k) { return {v.x/k, v.y/k, v.z/k, v.w/k}; }

  //Element-wise Multiplication
  template<typename T>
  gen_vec2<T> operator*(const gen_vec2<T> &a, const gen_vec2<T> &b) { return {a.x*b.x, a.y*b.y}; }

  template<typename T>
  gen_vec3<T> operator*(const gen_vec3<T> &a, const gen_vec3<T> &b) { return {a.x*b.x, a.y*b.y, a.z*b.z}; }

  template<typename T>
  gen_vec4<T> operator*(const gen_vec4<T> &a, const gen_vec4<T> &b) { return {a.x*b.x, a.y*b.y, a.z*b.z, a.w*b.w}; }

  //Element-wise Division
  template<typename T>
  gen_vec2<T> operator/(const gen_vec2<T> &a, const gen_vec2<T> &b) { return {a.x/b.x, a.y/b.y}; }

  template<typename T>
  gen_vec3<T> operator/(const gen_vec3<T> &a, const gen_vec3<T> &b) { return {a.x/b.x, a.y/b.y, a.z/b.z}; }

  template<typename T>
  gen_vec4<T> operator/(const gen_vec4<T> &a, const gen_vec4<T> &b) { return {a.x/b.x, a.y/b.y, a.z/b.z, a.w/b.w}; }
  
  //Dot Product
  template<typename T>
  T dot(const gen_vec2<T> &a, const gen_vec2<T> &b) { return a.x*b.x + a.y*b.y; }

  template<typename T>
  T dot(const gen_vec3<T> &a, const gen_vec3<T> &b) { return a.x*b.x + a.y*b.y + a.z*b.z; }

  template<typename T>
  T dot(const gen_vec4<T> &a, const gen_vec4<T> &b) { return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w; }
  
  /* Type-Specific Operations */
  
  float length(const float2 &v);
  float length(const float3 &v);
  float length(const float4 &v);

  float2 normalize(const float2 &v);
  float3 normalize(const float3 &v);
  float4 normalize(const float4 &v);
  
  float3 cross(const float3 &a, const float3 &b);

  void make_orthonormals(const float3 &N,
			 /* out */ float3 &T, /* out */ float3 &B);
  float3 spherical_direction(const float3 &N, const float3 &T, const float3 &B,
			     float sin_theta, float cos_theta, float phi);
  
};

#endif
