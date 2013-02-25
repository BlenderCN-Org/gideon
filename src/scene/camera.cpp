#include "scene/camera.hpp"

using namespace raytrace;

ray raytrace::camera_shoot_ray(const camera &cam, float x, float y) {
  float3 raster{x, y, 0.0f};

  ray r;
  r.o = cam.camera_to_world.apply_point({0.0f, 0.0f, 0.0f});
  r.d = cam.camera_to_world.apply_direction(normalize(cam.raster_to_camera.apply_perspective(raster)));
  r.min_t = cam.clip_start;
  r.max_t = cam.clip_end;

  return r;
}
