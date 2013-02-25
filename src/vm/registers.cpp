#include "vm/registers.hpp"
#include "scene/scene.hpp"

using namespace std;

namespace raytrace {
  
  /* floating point specializations */

  template<> float &register_file::get<float>(int i) { return f_data[i]; }
  template<> size_t register_file::capacity<float>() const { return f_data.size(); }
  
  template<> float2 &register_file::get<float2>(int i) { return f2_data[i]; }
  template<> size_t register_file::capacity<float2>() const { return f2_data.size(); }

  template<> float3 &register_file::get<float3>(int i) { return f3_data[i]; }
  template<> size_t register_file::capacity<float3>() const { return f3_data.size(); }
  
  template<> float4 &register_file::get<float4>(int i) { return f4_data[i]; }
  template<> size_t register_file::capacity<float4>() const { return f4_data.size(); }
  
  /* integer specializations */

  template<> int &register_file::get<int>(int i) { return i_data[i]; }
  template<> size_t register_file::capacity<int>() const { return i_data.size(); }

  template<> int2 &register_file::get<int2>(int i) { return i2_data[i]; }
  template<> size_t register_file::capacity<int2>() const { return i2_data.size(); }

  template<> int3 &register_file::get<int3>(int i) { return i3_data[i]; }
  template<> size_t register_file::capacity<int3>() const { return i3_data.size(); }

  template<> int4 &register_file::get<int4>(int i) { return i4_data[i]; }
  template<> size_t register_file::capacity<int4>() const { return i4_data.size(); }

  /* geometry type specializations */

  template<> ray &register_file::get<ray>(int i) { return ray_data[i]; }
  template<> size_t register_file::capacity<ray>() const { return ray_data.size(); }

  template<> intersection &register_file::get<intersection>(int i) { return isect_data[i]; }
  template<> size_t register_file::capacity<intersection>() const { return isect_data.size(); }

  /* misc types */

  template<> string &register_file::get<string>(int i) { return str_data[i]; }
  template<> size_t register_file::capacity<string>() const { return str_data.size(); }

  template<> object* &register_file::get<object*>(int i) { return obj_data[i]; }
  template<> size_t register_file::capacity<object*>() const { return obj_data.size(); }

  template<> primitive* &register_file::get<primitive*>(int i) { return prim_data[i]; }
  template<> size_t register_file::capacity<primitive*>() const { return prim_data.size(); }
 
  template<> void* &register_file::get<void*>(int i) { return ptr_data[i]; }
  template<> size_t register_file::capacity<void*>() const { return ptr_data.size(); }

  template<> shader_ref &register_file::get<shader_ref>(int i) { return shader_data[i]; }
  template<> size_t register_file::capacity<shader_ref>() const { return shader_data.size(); }
};
