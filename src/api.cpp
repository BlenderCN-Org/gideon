#include <iostream>
#include "scene/scene.hpp"
#include "geometry/triangle.hpp"
#include "scene/bvh.hpp"
#include "scene/bvh_builder.hpp"

#include "compiler/gd_std.hpp"

#include "vm/vm.hpp"
#include "vm/instruction.hpp"
#include "vm/standard.hpp"
#include "vm/program.hpp"

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

extern "C" void gde_isect_normal(intersection *i, scene_data *sdata, float3 *N) {
  scene *s = sdata->s;
  primitive &prim = s->primitives[i->prim_idx];
  int3 &tri = s->triangle_verts[prim.data_id];
  vector<float3> &verts = s->vertices;

  *N = compute_triangle_normal(verts[tri.x], verts[tri.y], verts[tri.z]);
}

extern "C" void gde_ray_point(ray *r, float t, float3 *P) {
  *P = r->point_on_ray(t);
}

extern "C" void gde_light_sample_position(light *lt, float3 *P, float rand_u, float rand_v,
					  float4 *P_out) {
  *P_out = lt->sample_position(*P, rand_u, rand_v);
}

extern "C" void gde_light_eval_radiance(light *lt, float3 *P, float3 *I, /* out */ float3 *R) {
  *R = lt->eval_radiance(*P, *I);
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

extern "C" int gde_scene_num_lights(scene_data *sdata) {
  return static_cast<int>(sdata->s->lights.size());
}

extern "C" void gde_scene_get_light(scene_data *sdata, int id, light **light_id) {
  *light_id = &sdata->s->lights[id];
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
  float *rgba_out = reinterpret_cast<float*>(out);
  int idx = 4 * (x + width*y);
  rgba_out[idx] = color->x;
  rgba_out[idx+1] = color->y;
  rgba_out[idx+2] = color->z;
  rgba_out[idx+3] = color->w;
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
  object *obj = s->objects[object_id];
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
  object *obj = s->objects[object_id];
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

extern "C" void gde_gen_ray(float3 *O, float3 *D, float max_t, ray *r) {
  *r = {*O, *D, 5.0f*epsilon, max_t + 5.0f*epsilon};
}

/* Code used by the Blender Python Plugin. */
extern "C" {

  void *rt_scene_init(void) {
    scene *s = new scene;
    cout << "Scene: " << s << endl;
    return (void*)(s);
  }

  void rt_scene_destroy(void *sptr) {
    scene *s = reinterpret_cast<scene*>(sptr);
    delete s;
  }

  int rt_scene_add_mesh(void *sptr,
			unsigned int num_verts, float *v_data,
			unsigned int num_triangles, int *t_data, int *s_data) {
    scene *s = reinterpret_cast<scene*>(sptr);
    
    /*cout << "Scene: " << s << " | N : " << num_verts << " | VData: " << v_data << endl;
    cout << "First: " << v_data[0] << endl;
    cout << "2: " << v_data[1] << endl;
    cout << "3: " << v_data[2] << endl;*/

    int vert_offset = s->vertices.size();
    int prim_offset = s->primitives.size();
    int tri_offset = s->triangle_verts.size();
    int object_id = s->objects.size();

    cout << "Sizes: [" << num_verts << ", " << num_triangles << "]" << endl;

    //load all vertices
    for (unsigned int i = 0; i < num_verts; i += 3) {
      /*cout << "i: " << i << endl;
      if (s->vertices.size() == 9) {
	cout << "Vertex " << s->vertices.size() << ": (" << v_data[i] << ", " << v_data[i+1] << ", " << v_data[i+2] << ")" << endl;
	}*/
      float3 v{v_data[i], v_data[i+1], v_data[i+2]};
      s->vertices.push_back(v);
    }

    //load all triangles, updating triangle vertex indices to match the global vertex array
    for (unsigned int i = 0; i < num_triangles; i += 3) {
      //cout << "triangle: " << i / 3 << endl;
      int tri_idx = static_cast<int>(s->triangle_verts.size());

      int3 t{t_data[i] + vert_offset, t_data[i+1] + vert_offset, t_data[i+2] + vert_offset};
      primitive p{primitive::PRIM_TRIANGLE, static_cast<int>(s->primitives.size()), tri_idx, object_id, -1, s_data[tri_idx]};
      
      s->triangle_verts.push_back(t);
      s->primitives.push_back(p);
    }

    //add the object
    int2 prim_range{prim_offset, static_cast<int>(s->primitives.size())};
    int2 vert_range{vert_offset, static_cast<int>(s->vertices.size())};
    int2 tri_range{tri_offset, static_cast<int>(s->triangle_verts.size())};
    object *o = new object;
    o->vert_range = vert_range;
    o->prim_range = prim_range;
    o->tri_range = tri_range;

    s->objects.push_back(o);
    return object_id;
  }

  void rt_add_texcoord(void *sptr, int object_id,
		       const char *name, float *uv_data, unsigned int N) {
    string uv_name = string("uv:") + string(name);
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

    s->objects[object_id]->attributes[uv_name] = attr;
  }

  void rt_add_vcolor(void *sptr, int object_id,
		     const char *name, float *c_data, unsigned int N) {
    string attr_name = string("attribute:") + string(name);
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

    s->objects[object_id]->attributes[attr_name] = attr;
  }

  void rt_scene_stats(void *sptr) {
    scene *s = reinterpret_cast<scene*>(sptr);

    cout << "Vertex Count: " << s->vertices.size() << endl;
    cout << "Triangle Count: " << s->triangle_verts.size() << endl;
    cout << "Primitive Count: " << s->primitives.size() << endl;
    cout << "Resolution: [" << s->resolution.x << ", " << s->resolution.y << "]" << endl;

    raytrace::transform id{
      {
	{1.0f, 0.0f, 0.0f, 0.0f},
	{0.0f, 1.0f, 0.0f, 0.0f},
	{0.0f, 0.0f, 1.0f, 0.0f},
	{0.0f, 0.0f, 0.0f, 1.0f}
      }
    };

    raytrace::transform translate{
      {
	{1.0f, 0.0f, 0.0f, 2.0f},
	{0.0f, 1.0f, 0.0f, 2.0f},
	{0.0f, 0.0f, 1.0f, -1.0f},
	{0.0f, 0.0f, 0.0f, 1.0f}
      }
    };

    float3 v{3.2f, 4.5f, 0.0f};
    float3 p = id.apply_point(v);
    cout << "P: {" << p.x << ", " << p.y << ", " << p.z << "}" << endl;

    p = translate.apply_point(v);
    cout << "P: {" << p.x << ", " << p.y << ", " << p.z << "}" << endl;

    p = translate.apply_perspective(v);
    cout << "P: {" << p.x << ", " << p.y << ", " << p.z << "}" << endl;

    p = translate.apply_direction(v);
    cout << "P: {" << p.x << ", " << p.y << ", " << p.z << "}" << endl;
  }

  void scene_add_lamp(void *sptr, float energy, float r, float g, float b, float radius,
		      float *position) {
    scene *s = reinterpret_cast<scene*>(sptr);

    float3 light_pt{position[0], position[1], position[2]};

    light lamp{light::POINT, {light_pt, radius},
	energy, float3{r, g, b}};
    s->lights.push_back(lamp);
  }

  void scene_set_camera(void *sptr,
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


  void scene_demo_render_prog(void *sptr,
			      /* out */ float *rgba_out) {
    InitializeNativeTarget();
    
    scene *s = reinterpret_cast<scene*>(sptr);

    cout << "Building scene BVH..." << endl;
    bvh accel = build_bvh_centroid_sah(s);
    cout << "...done." << endl;
    
    scene_data sd;
    sd.s = s;
    sd.accel = &accel;
    scene_data *sd_loc = &sd;

    uniform_real_distribution<float> uniform_dist(0.0f, 1.0f);
    mt19937 rand_engine;
    boost::function<float ()> rng = bind(uniform_dist, rand_engine);
    auto rng_ptr = &rng;

    render_program prog("demo_render");
    //prog.load_source_file("/home/curtis/Projects/relatively-crazy/tests/render_loop.gdl");
    prog.load_source_file("/home/curtis/Projects/relatively-crazy/posts/build-your-own-renderer-1/render_debug.gdl");
    Module *module = prog.compile();
    verifyModule(*module);
    module->dump();
    
    string load_err;
    sys::DynamicLibrary dylib = sys::DynamicLibrary::getPermanentLibrary("libraytrace.so", &load_err);
    cout << "Result of Library Load: " << load_err << endl;

    string error_str;
    ExecutionEngine *engine = EngineBuilder(module).setErrorStr(&error_str).create();
    engine->addGlobalMapping(cast<GlobalVariable>(module->getNamedGlobal(".__gd_scene")), (void*)&sd_loc);
    engine->addGlobalMapping(cast<GlobalVariable>(module->getNamedGlobal(".render.demo.__gd_output")), (void*)&rgba_out);
    //engine->addGlobalMapping(cast<GlobalVariable>(module->getNamedGlobal(".__gd_rng")), (void*)&rng_ptr);

    void *fptr = engine->getPointerToFunction(module->getFunction("gdi..2.main.i.i"));
    void (*render_entry)(int, int) = (void (*)(int, int))(fptr);

    render_entry(s->resolution.x, s->resolution.y);
    //render_entry(3, 3);

    delete engine;
  }

  void scene_demo_render_nice(void *sptr,
			      /* out */ float *rgba_out) {
    scene *s = reinterpret_cast<scene*>(sptr);
    unsigned int pixel_idx = 0;

    uniform_real_distribution<float> uniform_dist(0.0f, 1.0f);
    mt19937 engine;
    auto rng = bind(uniform_dist, engine);

    ray tmp = camera_shoot_ray(s->main_camera, 0, 0);
    printf("Ray: (%f, %f, %f) -> (%f, %f, %f)\n",
	   tmp.o.x, tmp.o.y, tmp.o.z, tmp.d.x, tmp.d.y, tmp.d.z);

    float3 camera_pt = s->main_camera.camera_to_world.apply_point({0.0f, 0.0f, 0.0f});
    float2 r_coord{160.0f, 120.0f};
    float4 pt = s->main_camera.raster_to_camera.apply({r_coord.x, r_coord.y, 0.0f, 1.0f});
    float3 pt1 = s->main_camera.raster_to_camera.apply_perspective({r_coord.x, r_coord.y, 0.0f});
    printf("Camera: (%f, %f, %f, %f)\n", pt.x, pt.y, pt.z, pt.w);
    printf("Camera': (%f, %f, %f)\n", pt1.x, pt1.y, pt1.z);
  
    cout << "Random: " << rng() << " | " << rng() << " | " << rng() << endl;
  
    //build a test bvh
    bvh accel = build_bvh_centroid_sah(s);

    //render the scene
    unsigned int aabb_total = 0;
    unsigned int prim_total = 0;
    unsigned int shadow_aabb = 0;
    unsigned int shadow_prim = 0;
    
    for (int i = 0; i < s->resolution.y; i++) {
      for (int j = 0; j < s->resolution.x; j++) {
	float depth = s->main_camera.clip_end;
	ray r = camera_shoot_ray(s->main_camera, j, i);
	intersection isect, nearest;

	unsigned int aabb_checked, prim_checked;
	if (accel.trace(r, nearest, aabb_checked, prim_checked)) depth = nearest.t;

	aabb_total += aabb_checked;
	prim_total += prim_checked;

	float exp_depth = expf(-0.2f*depth);
	float red = exp_depth;
	float green = exp_depth;
	float blue = exp_depth;

	if (depth < s->main_camera.clip_end) {
	  float3 isect_pt = r.point_on_ray(nearest.t);
	  primitive &prim = s->primitives[nearest.prim_idx];
	  int3 &tri = s->triangle_verts[prim.data_id];
	  vector<float3> &verts = s->vertices;

	  int object_id = prim.object_id;
	  object *obj = s->objects[object_id];

	  if (obj->attributes.find("uv:UVMap") != obj->attributes.end()) {
	    int attr_id = prim.data_id - obj->tri_range.x;
	    attribute *attr = obj->attributes["uv:UVMap"];
	    
	    float2 *coords = attr->data<float2>(attr_id);
	    
	    float inv = 1.0f - nearest.u - nearest.v;
	    float2 uv = nearest.u*coords[1] + nearest.v*coords[2] + inv*coords[0];
	    red = uv.x;
	    green = uv.y;
	  }

	  if (obj->attributes.find("attribute:my_colors") != obj->attributes.end()) {
	    int attr_id = prim.data_id - obj->tri_range.x;
	    attribute *attr = obj->attributes["attribute:my_colors"];
	    float3 *color = attr->data<float3>(attr_id);

	    float inv = 1.0f - nearest.u - nearest.v;
	    float3 c = nearest.u*color[1] + nearest.v*color[2] + inv*color[0];

	    red = c.x;
	    green = c.y;
	    blue = c.z;
	  }

	  
	  float3 N = compute_triangle_normal(verts[tri.x], verts[tri.y], verts[tri.z]);
	  float3 L{0.0f, 0.0f, 0.0f};
	  for (vector<light>::iterator l_it = s->lights.begin(); l_it != s->lights.end(); l_it++) {
	    unsigned int n_samples = 8;
	    float inv_N = 1.0f / n_samples;

	    for (unsigned int sample = 0; sample < n_samples; sample++) {
	      float4 tmp = l_it->sample_position(isect_pt, rng(), rng());
	      float3 P_light{tmp.x, tmp.y, tmp.z};
	      float3 I = normalize(P_light - isect_pt);
	      
	      //test for shadow
	      float dist_l = length(P_light - isect_pt);
	      ray shadow_ray{isect_pt, I, 5.0f*epsilon, dist_l + 5.0f*epsilon}; 
	      intersection shadow_isect;
	      
	      unsigned int aabb_count, prim_count;
	      if (!accel.trace(shadow_ray, shadow_isect, aabb_count, prim_count)) {
		//no hit -> not in shadow
		float fac = inv_N*std::max(0.0f, dot(N, I));
		L = L + fac*l_it->eval_radiance(P_light, I);
	      }

	      shadow_aabb += aabb_count;
	      shadow_prim += prim_count;
	    }
	  }

	  red *= L.x;
	  green *= L.y;
	  blue *= L.z;
	}
	
	rgba_out[pixel_idx++] = red; //depth;
	rgba_out[pixel_idx++] = green; //depth;
	rgba_out[pixel_idx++] = blue; //depth;
	rgba_out[pixel_idx++] = 1.0f;
      }
    }

    double num_pixels = s->resolution.y*s->resolution.x;
    cout << "Average AABB Checks: " << (aabb_total / num_pixels) << endl;
    cout << "Average Prim Checks: " << (prim_total / num_pixels) << endl;
    cout << "Average Shadow AABB Checks: " << (shadow_aabb / num_pixels) << endl;
    cout << "Average Shadow Prim Checks: " << (shadow_prim / num_pixels) << endl;
  }
  
  void scene_demo_render(void *sptr, /* out */ float *rgba_out) {
    scene_demo_render_prog(sptr, rgba_out);
    //scene_demo_render_nice(sptr, rgba_out);
  }

};
