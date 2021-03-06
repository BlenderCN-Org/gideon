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

#include <iostream>
#include "scene/scene.hpp"
#include "geometry/triangle.hpp"
#include "scene/bvh.hpp"
#include "scene/bvh_builder.hpp"

#include "compiler/gd_std.hpp"

#include "engine/context.hpp"

#include "compiler/rendermodule.hpp"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/JIT.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/DynamicLibrary.h"

#include <dlfcn.h>

#include <random>
#include <functional>

#include <math.h>

#include <fstream>
#include <sstream>

using namespace std;
using namespace raytrace;
using namespace llvm;
using namespace gideon::rl;
using namespace gideon;

#define GDRL_DLL_EXPORT __attribute__ ((visibility ("default")))

extern "C" GDRL_DLL_EXPORT void gde_print_coords(int x, int y) {
  cout << "Coordinates (" << x << ", " << y << ")" << endl;
}

struct sdata {
  char is_const;
  char *data;
};
extern "C" void gde_print_string(sdata *str) {
  cout << str->data << endl;
}



extern "C" void gde_ray_point(ray *r, float t, float3 *P) {
  *P = r->point_on_ray(t);
}



extern "C" void gde_dir_to_point(float4 *P, float3 *O, /* out */ float3 *D, /* out */ float3 *nD) {
  *D = normalize(*O - float3{P->x, P->y, P->z});
  *nD = -1.0f * (*D);
}

extern "C" void gde_vec3_scale(float3 *v, float k, float3 *result) {
  *result = k * (*v);
}

extern "C" void gde_vec3_comp_mul(float3 *lhs, float3 *rhs, float3 *result) {
  *result = (*lhs) * (*rhs);
}

extern "C" void gde_vec4_to_vec3(float4 *v4, float3 *v3) {
  *v3 = float3{v4->x, v4->y, v4->z};
}

extern "C" float gde_gen_random(boost::function<float ()> *rng) {
  return (*rng)();
};

extern "C" int gde_draw_coords(int x, int y, int idx, float3 *color, void *out) {
  float *rgba_out = reinterpret_cast<float*>(out);

  rgba_out[idx] = color->x;
  rgba_out[idx+1] = color->y;
  rgba_out[idx+2] = color->z;
  rgba_out[idx+3] = 1.0f;
  return idx + 4;
}

extern "C" void gde_draw_pixel(int x, int y, int width, int height, float4 *color, void *out) {
  typedef float buffer_elem_type[4];
  float (*buffer)[4] = reinterpret_cast<buffer_elem_type*>(out);
  int idx = x + width*y;
  float *pix = buffer[idx];
  pix[0] = color->x;
  pix[1] = color->y;
  pix[2] = color->z;
  pix[3] = color->w;
}

extern "C" void gde_print_stats(int aabb, int prim, int num_pixels) {
  float avg_aabb = static_cast<float>(aabb) / num_pixels;
  float avg_prim = static_cast<float>(prim) / num_pixels;

  cout << "Bounding Boxes Checked: " << avg_aabb << " boxes / pixel" << endl;
  cout << "Primitives Checked: " << avg_prim << " triangles / pixel" << endl;
}

float interpolate(float val, float y0, float x0, float y1, float x1) {
  return ((val - x0) * (y1 - y0) / (x1 - x0)) + y0;
}

float jet_base(float val) {
  if (val <= -0.75f) return 0.0f;
  if (val <= -0.25f) return interpolate(val, 0.0f, -0.75f, 1.0f, -0.25f);
  if (val <= 0.25f) return 1.0f;
  if (val <= 0.75f) return interpolate(val, 1.0f, 0.25f, 0.0f, 0.75f);
  return 0.0f;
}

extern "C" void gde_stats_to_color(int count, float k, float4 *out) {
  float d = count;
  d = 1.0f - expf(-k * d);

  float d2 = (2.0f * d) - 1.0f;

  out->x = jet_base(d2 - 0.5f);
  out->y = jet_base(d2);
  out->z = jet_base(d2 + 0.5f);
  out->w = 1.0f;
}

extern "C" float gde_scene_trace(ray *r, void *sptr) {
  scene_data *sdata = reinterpret_cast<scene_data*>(sptr);
  scene *s = sdata->s;
  bvh &accel = *sdata->accel;

  float depth = s->main_camera.clip_end;
  intersection nearest;
  
  unsigned int aabb_checked, prim_checked;
  if (accel.trace(*r, nearest, aabb_checked, prim_checked)) depth = nearest.t;
  return depth;
}

extern "C" bool gde_isect_get_attribute3(intersection *i,
					 sdata *name, scene_data *sdata,
					 /* out */ float3 *color) {
  string attr_name(name->data);
  scene *s = sdata->s;

  primitive &prim = s->primitives[i->prim_idx];
  int3 &tri = s->triangle_verts[prim.data_id];
  vector<float3> &verts = s->vertices;

  int object_id = prim.object_id;
  object_ptr obj = s->objects[object_id];
  if (obj->attributes.find(attr_name) == obj->attributes.end()) return false;

  int attr_id = prim.data_id - obj->tri_range.x;
  attribute *attr = obj->attributes[attr_name];
  float3 *val = attr->data<float3>(attr_id);

  float inv = 1.0f - i->u - i->v;
  *color = i->u*val[1] + i->v*val[2] + inv*val[0];
  return true;
}

extern "C" bool gde_isect_get_attribute2(intersection *i,
					 sdata *name, scene_data *sdata,
					 /* out */ float2 *color) {
  string attr_name(name->data);
  scene *s = sdata->s;

  primitive &prim = s->primitives[i->prim_idx];
  int3 &tri = s->triangle_verts[prim.data_id];
  vector<float3> &verts = s->vertices;

  int object_id = prim.object_id;
  object_ptr obj = s->objects[object_id];
  if (obj->attributes.find(attr_name) == obj->attributes.end()) return false;

  int attr_id = prim.data_id - obj->tri_range.x;
  attribute *attr = obj->attributes[attr_name];
  float2 *val = attr->data<float2>(attr_id);

  float inv = 1.0f - i->u - i->v;
  *color = i->u*val[1] + i->v*val[2] + inv*val[0];
  return true;
}

extern "C" void gde_print_ray(ray *r) {
  cout << "Origin: (" << r->o.x << ", " << r->o.y << ", " << r->o.z << ")" << endl;
  cout << "Direction: (" << r->d.x << ", " << r->d.y << ", " << r->d.z << ")" << endl;
}

extern "C" float gde_int_to_float(int i) { return i; }
extern "C" int gde_floor_int(float f) { return static_cast<int>(floorf(f)); }

/* Code used by the Blender Python Plugin. */
extern "C" {
  
  /* Context Management */

  void gd_api_initialize(const char *lib_name) {
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();
  
    string load_err;
    sys::DynamicLibrary dylib = sys::DynamicLibrary::getPermanentLibrary(lib_name, &load_err);
    cout << "Library Load: " << load_err << endl;
  }

  void *gd_api_create_context() {
    return reinterpret_cast<void*>(new render_context);
  }

  void gd_api_destroy_context(void *ctx_ptr) {
    render_context *ctx = reinterpret_cast<render_context*>(ctx_ptr);
    delete ctx;
  }

  void gd_api_context_set_kernel(void *ctx_ptr, void *kernel_ptr) {
    render_context *ctx = reinterpret_cast<render_context*>(ctx_ptr);
    ctx->set_kernel(unique_ptr<render_kernel>(reinterpret_cast<render_kernel*>(kernel_ptr)));
  }

  void *gd_api_context_get_kernel(void *ctx_ptr) {
    render_context *ctx = reinterpret_cast<render_context*>(ctx_ptr);
    return reinterpret_cast<void*>(ctx->get_kernel());
  }

  void gd_api_context_set_scene(void *ctx_ptr, void *scene_ptr) {
    render_context *ctx = reinterpret_cast<render_context*>(ctx_ptr);
    ctx->set_scene(unique_ptr<scene>(reinterpret_cast<scene*>(scene_ptr)));
  }

  void gd_api_context_build_bvh(void *ctx_ptr) {
    render_context *ctx = reinterpret_cast<render_context*>(ctx_ptr);
    ctx->build_bvh();
  }

  /* String Allocation */

  //Makes a new copy of the provided string, allocated with new[].
  char *gd_api_string_copy(const char *s) {
    char *buf = new char[strlen(s)+1];
    strcpy(buf, s);
    return buf;
  }

  //Easy way to set return status values from Python.
  void gd_api_set_status(/* out */ void *result, int status) {
    int *r = reinterpret_cast<int*>(result);
    *r = status;
  }

  /* Program Management */

  void *gd_api_create_program(const char *name,
			      void *resolve_cb, void *load_cb) {
    typedef char *(*gd_api_source_load_cb_type)(const char *, void*);
    typedef char *(*gd_api_path_resolve_cb_type)(const char *, void*);
    
    auto resolve_func = [resolve_cb] (const string &path) -> string {
      int status = 0;
      char *new_str = ((gd_api_path_resolve_cb_type)(resolve_cb))(path.c_str(), (void*)&status);
      if (status == 0) {
	throw runtime_error(string("Unable to resolve pathname: ") + path);
      }
      
      string result = new_str;
      delete[] new_str;
      return result;
    };

    auto loader_func = [load_cb] (const string &path) -> string {
      int status = 0;
      char *new_str = ((gd_api_source_load_cb_type)(load_cb))(path.c_str(), (void*)&status);
      if (status == 0) {
	throw runtime_error(string("Unable to load source: ") + path);
      }
      
      string source = new_str;
      delete[] new_str;
      return source;
    };
    
    return reinterpret_cast<void*>(new render_program(name,
						      true, false,
						      resolve_func, loader_func));
  }

  void gd_api_destroy_program(void *p) {
    render_program *prog = reinterpret_cast<render_program*>(p);
    delete prog;
  }
  
  void gd_api_program_load_source(void *p, const char *fname) {
    render_program *prog = reinterpret_cast<render_program*>(p);
    prog->load_source_file(fname);
  }

  typedef void (*compiler_error_cb)(const char *);
  void *gd_api_program_compile(void *p, compiler_error_cb error_cb) {
    try {
      render_program *prog = reinterpret_cast<render_program*>(p);
      Module *module = prog->compile();
      //module->dump();
      
      return reinterpret_cast<void*>(new compiled_renderer(module));
    }
    catch (runtime_error &e) {
      error_cb(e.what());
      return NULL;
    }
  }

  void gd_api_list_material_functions(void *p, void *on_func_cb) {
    typedef void (*func_list_cb_type)(const char *, const char *);
    render_program *prog = reinterpret_cast<render_program*>(p);

    auto on_func = [on_func_cb] (const string &name, const string &full_name) -> void {
      ((func_list_cb_type)(on_func_cb))(name.c_str(), full_name.c_str());
    };

    prog->foreach_function_type(exports::function_export::export_type::MATERIAL,
				on_func);
  }

  void gd_api_list_entry_functions(void *p, void *on_func_cb) {
    typedef void (*func_list_cb_type)(const char *, const char *);
    render_program *prog = reinterpret_cast<render_program*>(p);

    auto on_func = [on_func_cb] (const string &name, const string &full_name) -> void {
      ((func_list_cb_type)(on_func_cb))(name.c_str(), full_name.c_str());
    };

    prog->foreach_function_type(exports::function_export::export_type::ENTRY,
				on_func);
  }

  void gd_api_destroy_renderer(void *r) {
    compiled_renderer *render = reinterpret_cast<compiled_renderer*>(r);
    delete render;
  }

  void *gd_api_lookup_function(void *r, const char *fname) {
    compiled_renderer *render = reinterpret_cast<compiled_renderer*>(r);
    return render->get_function_pointer(fname);
  }

  /* Scene Management */

  void *gd_api_create_scene() {
    return reinterpret_cast<void*>(new scene);
  }

  void gd_api_destroy_scene(void *s) {
    scene *scn = reinterpret_cast<scene*>(s);
    delete scn;
  }

  int gd_api_add_mesh(void *sptr,
		      unsigned int num_verts, float *v_data, float *v_norm_data,
		      unsigned int num_triangles, int *t_data,
		      void **mat_data, void **volume_data) {
    scene *s = reinterpret_cast<scene*>(sptr);
    
    int vert_offset = s->vertices.size();
    int prim_offset = s->primitives.size();
    int tri_offset = s->triangle_verts.size();
    int object_id = s->objects.size();

    //load all vertices
    for (unsigned int i = 0; i < num_verts; i += 3) {
      float3 v{v_data[i], v_data[i+1], v_data[i+2]};
      float3 vn{v_norm_data[i], v_norm_data[i+1], v_norm_data[i+2]};

      s->vertices.push_back(v);
      s->vertex_normals.push_back(vn);
    }

    //load all triangles, updating triangle vertex indices to match the global vertex array
    for (unsigned int i = 0; i < num_triangles; i += 3) {
      int tri_idx = static_cast<int>(s->triangle_verts.size());
      int mat_idx = i / 3;

      int3 t{t_data[i] + vert_offset, t_data[i+1] + vert_offset, t_data[i+2] + vert_offset};

      primitive p{primitive::PRIM_TRIANGLE, static_cast<int>(s->primitives.size()), tri_idx, object_id, -1, mat_data[mat_idx], volume_data[mat_idx]};
      
      s->triangle_verts.push_back(t);
      s->primitives.push_back(p);
    }

    //add the object
    int2 prim_range{prim_offset, static_cast<int>(s->primitives.size())};
    int2 vert_range{vert_offset, static_cast<int>(s->vertices.size())};
    int2 tri_range{tri_offset, static_cast<int>(s->triangle_verts.size())};
    object_ptr o = object_ptr(new object);
    o->vert_range = vert_range;
    o->prim_range = prim_range;
    o->tri_range = tri_range;

    s->objects.push_back(o);
    return object_id;
  }

  void gd_api_add_texcoord(void *sptr, int object_id,
			   const char *name, float *uv_data, unsigned int N) {
    scene *s = reinterpret_cast<scene*>(sptr);
    attribute *attr = new attribute(attribute::PER_CORNER, attribute_type{attribute_type::FLOAT, attribute_type::VEC2});

    unsigned int num_elements = N/6; //2 coords per item, 3 items per element
    attr->resize(num_elements);
    
    for (unsigned int i = 0; i < N; i += 6) {      
      int elem_id = i / 6;
      float2 *coord = attr->data<float2>(elem_id);

      coord[0] = float2{uv_data[i], uv_data[i+1]};
      coord[1] = float2{uv_data[i+2], uv_data[i+3]};
      coord[2] = float2{uv_data[i+4], uv_data[i+5]};
    }

    s->objects[object_id]->attributes[name] = attr;
  }

  void gd_api_add_vertex_color(void *sptr, int object_id,
			       const char *name, float *c_data, unsigned int N) {
    scene *s = reinterpret_cast<scene*>(sptr);

    attribute *attr = new attribute(attribute::PER_CORNER, attribute_type{attribute_type::FLOAT, attribute_type::VEC3});
    unsigned int num_elements = N / 9;
    attr->resize(num_elements);

    for (unsigned int i = 0; i < N; i += 9) {
      int elem_id = i / 9;
      float3 *color = attr->data<float3>(elem_id);

      color[0] = float3{c_data[i], c_data[i+1], c_data[i+2]};
      color[1] = float3{c_data[i+3], c_data[i+4], c_data[i+5]};
      color[2] = float3{c_data[i+6], c_data[i+7], c_data[i+8]};
    }

    s->objects[object_id]->attributes[name] = attr;
  }

  void gd_api_set_camera(void *sptr,
			 int resolution_x, int resolution_y,
			 float clip_start, float clip_end,
			 float *camera_to_world_4x4,
			 float *raster_to_camera_4x4) {
    scene *s = reinterpret_cast<scene*>(sptr);
    
    s->main_camera.clip_start = clip_start;
    s->main_camera.clip_end = clip_end;
    
    s->main_camera.camera_to_world = {{
	{camera_to_world_4x4[0], camera_to_world_4x4[1], camera_to_world_4x4[2], camera_to_world_4x4[3]},
	{camera_to_world_4x4[4], camera_to_world_4x4[5], camera_to_world_4x4[6], camera_to_world_4x4[7]},
	{camera_to_world_4x4[8], camera_to_world_4x4[9], camera_to_world_4x4[10], camera_to_world_4x4[11]},
	{camera_to_world_4x4[12], camera_to_world_4x4[13], camera_to_world_4x4[14], camera_to_world_4x4[15]}
      }};
    
    s->main_camera.raster_to_camera = {{
	{raster_to_camera_4x4[0], raster_to_camera_4x4[1], raster_to_camera_4x4[2], raster_to_camera_4x4[3]},
	{raster_to_camera_4x4[4], raster_to_camera_4x4[5], raster_to_camera_4x4[6], raster_to_camera_4x4[7]},
	{raster_to_camera_4x4[8], raster_to_camera_4x4[9], raster_to_camera_4x4[10], raster_to_camera_4x4[11]},
	{raster_to_camera_4x4[12], raster_to_camera_4x4[13], raster_to_camera_4x4[14], raster_to_camera_4x4[15]}
      }};
    
    s->resolution = {resolution_x, resolution_y};
  }

  void gd_api_add_lamp(void *sptr, float energy, float r, float g, float b, float radius,
		       float *position) {
    scene *s = reinterpret_cast<scene*>(sptr);
    
    float3 light_pt{position[0], position[1], position[2]};
    
    light lamp{
      light::POINT,
	{{light_pt, radius}},
	energy, float3{r, g, b}
    };
    s->lights.push_back(lamp);
  }

  void *gd_api_build_bvh(void *s) {
    scene *scn = reinterpret_cast<scene*>(s);
    cout << "Building Scene BVH..." << endl;

    bvh *accel = new bvh(build_bvh_centroid_sah(scn));

    cout << "...done." << endl;
    return reinterpret_cast<void*>(accel);
  }

  void gd_api_destroy_bvh(void *b) {
    bvh *accel = reinterpret_cast<bvh*>(b);
    delete accel;
  }

  /* Rendering */

  void gd_api_render_tile(void *ctx_ptr, const char *entry_name,
			  int x, int y, int w, int h,
			  float (*output_buffer)[4]) {
    render_context *ctx = reinterpret_cast<render_context*>(ctx_ptr);

    void *entry_ptr = ctx->get_kernel()->get_function_pointer(entry_name);    
    void (*entry)(int, int, int, int, void*) = (void (*)(int, int, int, int, void*))(entry_ptr);
    entry(x, y, w, h, reinterpret_cast<void*>(output_buffer));
  }
};
