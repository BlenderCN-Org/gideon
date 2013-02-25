#include <iostream>
#include "scene/scene.hpp"
#include "geometry/triangle.hpp"
#include "scene/bvh.hpp"
#include "scene/bvh_builder.hpp"
#include "vm/vm.hpp"
#include "vm/instruction.hpp"
#include "vm/standard.hpp"
#include "vm/program.hpp"

#include <random>
#include <functional>

#include <math.h>

using namespace std;
using namespace raytrace;

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

    transform id{
      {
	{1.0f, 0.0f, 0.0f, 0.0f},
	{0.0f, 1.0f, 0.0f, 0.0f},
	{0.0f, 0.0f, 1.0f, 0.0f},
	{0.0f, 0.0f, 0.0f, 1.0f}
      }
    };

    transform translate{
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
    scene *s = reinterpret_cast<scene*>(sptr);

    cout << "Building scene BVH..." << endl;
    bvh accel = build_bvh_centroid_sah(s);
    cout << "...done." << endl;

    vm svm(s, &accel);
    svm.output.rgba_out = rgba_out;
    svm.output.pixel_idx = 0;

    instruction_set is = create_standard_instruction_set();

    /* build a simple distribution function */
    program reflect;
    reflect.constants.get<float3>(0) = {1.0f, 1.0f, 1.0f};
    reflect.constants.get<float>(0) = 1.0f;
    
    reflect.constants.get<int>(0) = 0;

    vector<string> bsdf_code({
	//read sigma parameter
	"movi_f_iC 0 0",

	  //evaluate the bsdf
	  "debug_oren_nayar 0 0 0 2 4",
	  
	  //multiply it by the color
	  "mul_f_f3C 5 0 0",
	  
	  //copy out elements and build result
	  "vector_elem_f3 0 5 0",
	  "vector_elem_f3 1 5 1",
	  "vector_elem_f3 2 5 2",
	  "mov_f_fC 3 0",
	  "merge4_f 0 0 1 2 3" //merge all components with 1.0 alpha and save to result register
	  });
    
    assemble(is, reflect.code, bsdf_code);
    s->distributions.push_back(distribution_function("oren-nayar", distribution_function::DIFFUSE,
						     &reflect, 0, 0,
						     {{"sigma", sizeof(float)}}));

    cout << "Distribution Parameter Size: " << s->distributions[0].param_size() << endl;


    program render;

    parameter_list results(2*sizeof(int));

    //setup constants
    render.constants.get<int>(0) = s->resolution.x;
    render.constants.get<int>(1) = s->resolution.y;
    render.constants.get<int>(2) = 0;
    render.constants.get<int>(3) = 1;
    render.constants.get<int>(7) = sizeof(int);
    render.constants.get<int>(8) = 0; //ID of the distribution
    render.constants.get<int>(9) = 12; //# of shadow samples per light

    render.constants.get<float>(0) = s->resolution.x;
    render.constants.get<float>(1) = s->resolution.y;
    render.constants.get<float>(2) = s->main_camera.clip_end;
    render.constants.get<float>(3) = 0.0f;

    render.constants.get<float>(4) = epsilon;
    render.constants.get<float>(5) = 1.0f;

    render.constants.get<float>(6) = 0.8f; //shading parameter - sigma
    
    render.constants.get<float3>(0) = {0.0f, 0.0f, 1.0f};
    render.constants.get<float3>(1) = {0.0f, 0.0f, 0.0f};

    render.constants.get<string>(0) = "attribute:my_colors";
    render.constants.get<string>(1) = "uv:UVMap";
    render.constants.get<string>(2) = "Number of Lights: ";

    vector<string> radiance_func({
	//-- light shading routine
	//f3R(-2) = position to illuminate
	//f3R(-3) = shading location normal
	//f3R(-4) = color at shading location
	//f3R(-5) = direction of eye
	//returns f3R(-1)

	//instantiate the bsdf
	"mov_i_iC 8 8",
	  "mov_f_fC 0 6",

	  "distribution_create 0 8",
	  "distribution_get_params 1 0",
	  "movp_f_iC 1 0 2", //copy sigma to input
	  "shader_scale_f3 0 -4 0", //scale the shader by the surface color
	  
	  "mov_f3_f3C -1 1",
	  
	  "mov_i_iC 2 9",
	  "conv_i_to_f 5 2",
	  "mov_f_fC 6 5",
	  "div_f_f 6 6 5", //1 / num_samples

	  "light_count 2",
	  "mov_i_iC 3 2", //loop counter = 0
	  "save_pc_offset 4 1",
	  
	  "mov_i_iC 9 2", //sample counter = 0
	  "save_pc_offset 10 1",

	  //sample a location
	  "mov_f_fC 0 3",
	  "random_f 2",
	  "random_f 3",
	  "light_sample_position 0 3 -2 2 3",
	  
	  //remove w-component (light sample returns float4)
	  "vector_elem_f4 2 0 0",
	  "vector_elem_f4 3 0 1",
	  "vector_elem_f4 4 0 2",
	  "merge3_f 0 2 3 4",
	  
	  //build illumination direction & ray
	  "sub_f3_f3 1 0 -2",
	  "normalize_f3 2 1",

	  //setup ray bounds
	  "mov_f_fC 1 4", //near - epsilon
	  "length_f3 2 1", //far - distance to light source + epsilon
	  "add_f_fC 2 2 4",
	  
	  "build_ray 0 -2 2 1 2",
	  
	  //cast shadow ray
	  "trace 6 0 0 7 8",
	  
	  //if not in shadow - evaluate illumination
	  "save_pc_offset 7 11",
	  "jump_nz_R 6 7",
	  
	  //evaluate the surface's bsdf at this point
	  "shader_eval 1 0 -3 -2 -2 2 -5",
	  "vector_elem_f4 2 1 0",
	  "vector_elem_f4 3 1 1",
	  "vector_elem_f4 4 1 2",
	  "merge3_f 4 2 3 4",
	  
	  "light_eval_radiance 3 3 0 2", //evaluate incoming radiance
	  "elem_mul_f3_f3 3 3 4", //multiply by illumination factor
	  "mul_f_f3 3 6 3", //scale by sample count
	  "add_f3_f3 -1 -1 3", //add to result

	  "add_i_iC 9 9 3", //sample counter++
	  "sub_i_iC 5 9 9", //tmp = sample_counter - num_samples
	  "jump_nz_R 5 10",
	  
	  "add_i_iC 3 3 3", //counter++
	  "sub_i_i 5 3 2", //tmp = counter - light_count
	  "jump_nz_R 5 4",
	  
	  "jump_R 1", //return
	  });

    vector<string> shader({
	//-- shading routine
	//isectR(-1) - ray intersection
	//rayR(-1) - ray
	//returns f3R(-1)
	
	//access vertex color
	"isect_u_v -1 1 2",
	  "mov_f_fC 3 3",
	  "merge3_f 124 1 2 3",
	  "isect_primitive 0 -1",
	  "isect_depth -1 -1",
	  "prim_attribute_f3 3 124 0 0 1 2", //fR3 f3R0 primR0 stringC0 fR1 fR2
	  
	  //check for a UVMap attribute too
	  "prim_attribute_f2 3 0 0 1 1 2",
	  
	  "save_pc_offset 2 5",
	  "jump_z_R 3 2",
	  
	  "vector_elem_f2 1 0 0",
	  "vector_elem_f2 2 0 1",
	  "merge3_f 124 1 2 3",
	  
	  //gather illumination from the lights
	  "prim_geometry_normal 125 0 1 2",
	  "point_on_ray 126 -1 -1", 
	  "ray_dir 123 -1",
	  
	  "stack_push",
	  "mov_i_iC 0 6", //load function address
	  "save_pc_offset 1 2",
	  "jump_R 0",
	  "stack_pop",
	  
	  "mov_f3_f3 -1 127",
	  
	  //compute the number of lights
	  "light_count 3",
	  "debug_to_string_i 0 3",
	  "mov_s_sC 1 2",
	  "string_concat 0 1 0",
	  //"debug_print_str 0",
	  
	  "jump_R 1",
	  });

    vector<string> pixel_draw({
	//-- pixel draw routine
	//fR(-1) - pixel x coordinate
	//fR(-2) - pixel y coordinate
	//iR(-1) - aabb test count (in/out)
	//iR(-2) - primitive test count (in/out)
	
	"gen_camera_ray 0 -1 -2",
	  "trace 2 0 127 3 4",
	  "mov_ray_ray 127 0",
	  
	  //record statistics
	  "add_i_i -1 -1 3",
	  "add_i_i -2 -2 4",
	  
	  "save_pc_offset 3 10",
	  "jump_z_R 2 3", //skip the next few instructions if the ray didn't hit
	  
	  //ray-hit something - let's shade it
	  "stack_push",
	  "mov_i_iC 0 5", //load function address
	  "save_pc_offset 1 2",
	  "jump_R 0",
	  "stack_pop",
	  "mov_f3_f3 0 127", //copy the returned color into the register that will be drawn		   
	  
	  "save_pc_offset 3 4",
	  "jump_R 3",
	  
	  //did not hit, so write black
	  "mov_f_fC 0 3",
	  "merge3_f 0 0 0 0",
	  
	  "debug_write_color 0", //write to the output buffer
	  
	  //return
	  "jump_R 1",
      });
    
    //define a program to trace a ray at each point and record depth
    vector<string> code({
	//-- main loop start
	
	//initialize statistics
	"mov_i_iC 127 2",
	  "mov_i_iC 126 2",
	  
	  //set the loop counters
	  "save_pc_offset 3 2",
	  "mov_i_iC 1 2",
	  
	  "save_pc_offset 4 2",
	  "mov_i_iC 0 2",

	  //get pixel float coordinates
	  "conv_i_to_f 127 0",
	  "conv_i_to_f 126 1",

	  //"debug_print 0 1",
	  
	  //call render function
	  "stack_push",
	  "mov_i_iC 0 4", //load function address
	  "save_pc_offset 1 2", //save return address
	  "jump_R 0", //execute the function
	  "stack_pop",
	  
	  //next pixel
	  "add_i_iC 0 0 3", //x++
	  "sub_i_iC 2 0 0", //(x - resolution_x)
	  "jump_nz_R 2 4",
	  
	  //next column
	  "add_i_iC 1 1 3", //y++
	  "sub_i_iC 2 1 1", //(y - resolution_y)
	  "jump_nz_R 2 3",
	  
	  //all done, write statistics to output
	  "movo_i_iC 127 2",
	  "movo_i_iC 126 7",
	  "jump -1",
	  });
    
    render.constants.get<int>(4) = static_cast<int>(code.size()); //location of the pixel draw routine
    code.insert(code.end(), pixel_draw.begin(), pixel_draw.end());

    render.constants.get<int>(5) = static_cast<int>(code.size()); //location of the shader routine
    code.insert(code.end(), shader.begin(), shader.end());

    render.constants.get<int>(6) = static_cast<int>(code.size()); //location of the lighting routine
    code.insert(code.end(), radiance_func.begin(), radiance_func.end());
    
    assemble(is, render.code, code);
    
    cout << "Num Instructions: " << render.code.size() << endl;
    svm.execute(render, 0, NULL, &results);

    cout << "Total Number AABB Checks: " << results.get<int>(0) << endl;
    cout << "Total Number Prim Checks: " << results.get<int>(sizeof(int)) << endl;
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
