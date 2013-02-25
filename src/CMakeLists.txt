
set(RT_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)

set(RT_SOURCE_FILES
  api.cpp

  math/vector.cpp
  math/transform.cpp
  math/differential.cpp
  math/sampling.cpp
  math/random.cpp

  geometry/aabb.cpp
  geometry/triangle.cpp

  scene/camera.cpp
  scene/primitive.cpp
  scene/bvh.cpp
  scene/bvh_builder.cpp
  scene/attribute.cpp
  scene/object.cpp
  scene/scene.cpp
  scene/light.cpp

  vm/instruction.cpp
  vm/standard.cpp

  vm/instructions/standard/arithmetic.cpp
  vm/instructions/standard/control.cpp
  vm/instructions/standard/data.cpp
  vm/instructions/standard/attributes.cpp
  vm/instructions/standard/geometry.cpp
  vm/instructions/standard/shading.cpp
  vm/instructions/standard/query.cpp
  vm/instructions/standard/debug.cpp

  vm/registers.cpp
  vm/parameters.cpp
  vm/vm.cpp

  shading/distribution.cpp
  shading/shadetree.cpp
  )

include_directories(${RT_INCLUDE_DIR})
add_library(raytrace SHARED ${RT_SOURCE_FILES})
#add_executable(raytrace ${RT_SOURCE_FILES} ${CMAKE_SOURCE_DIR}/tests/test_main.cpp)
set_target_properties(raytrace
  PROPERTIES
  COMPILE_FLAGS "-std=c++0x"
  )