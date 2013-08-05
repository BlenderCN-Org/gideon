include(external)

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

  engine/context.cpp

  shading/distribution.cpp

  compiler/rendermodule.cpp
  compiler/types.cpp
  compiler/errors.cpp
  compiler/debug.cpp
  compiler/symboltable.cpp
  compiler/operations.cpp
  compiler/type_conversion.cpp
  compiler/llvm_helper.cpp
  compiler/gen_state.cpp
  compiler/lookup.cpp
  compiler/value.cpp

  compiler/builtin_ops.cpp  
  compiler/gd_std.cpp

  compiler/types/primitive.cpp
  compiler/types/special.cpp
  compiler/types/vector.cpp
  compiler/types/array.cpp

  compiler/ast/ast.cpp
  compiler/ast/node.cpp
  compiler/ast/literal.cpp
  compiler/ast/expression.cpp
  compiler/ast/variable.cpp
  compiler/ast/assignment.cpp
  compiler/ast/function.cpp
  compiler/ast/statement.cpp
  compiler/ast/control.cpp
  compiler/ast/distribution.cpp
  compiler/ast/module.cpp
  compiler/ast/alias.cpp
  compiler/ast/typename.cpp
  compiler/ast/error.cpp
  )

find_package(BISON)
find_package(FLEX)
bison_target(rtl_parser compiler/language/rtlparser.y ${CMAKE_CURRENT_BINARY_DIR}/rtlparser.cpp COMPILE_FLAGS "--verbose")
flex_target(rtl_scanner compiler/language/rtlscanner.l ${CMAKE_CURRENT_BINARY_DIR}/rtlscanner.cpp COMPILE_FLAGS "--header-file=${CMAKE_CURRENT_BINARY_DIR}/rtlscanner.hpp" )
add_flex_bison_dependency(rtl_scanner rtl_parser)

set(RT_PARSER_LEXER_SOURCE ${BISON_rtl_parser_OUTPUTS} ${FLEX_rtl_scanner_OUTPUTS})

include_directories(${RT_INCLUDE_DIR} ${LLVM_INCLUDE_DIRS} ${OIIO_INCLUDE_DIR} ${Boost_INCLUDE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
link_directories(${LLVM_LIBRARY_DIRS} ${OIIO_LIBRARY_DIRS})
add_definitions(${LLVM_DEFINITIONS})

add_library(gideon SHARED ${RT_SOURCE_FILES} ${RT_PARSER_LEXER_SOURCE})
add_executable(gideon_compiler ${CMAKE_SOURCE_DIR}/tests/test_main.cpp)

target_link_libraries(gideon ${LLVM_LIBRARIES} ${Boost_LIBRARIES} ${OIIO_LIBRARIES} pthread m dl)
target_link_libraries(gideon_compiler gideon)

set_target_properties(gideon
  PROPERTIES
  COMPILE_FLAGS "-std=c++11"
  )

set_target_properties(gideon_compiler
  PROPERTIES
  COMPILE_FLAGS "-std=c++11"
  )

#Install the library and the python code to Blender's addon directory.
install(TARGETS gideon LIBRARY DESTINATION "${BLENDER_ADDON_ROOT}/gideon")
install(DIRECTORY "${CMAKE_SOURCE_DIR}/blender/"
  DESTINATION "${BLENDER_ADDON_ROOT}/gideon"
  PATTERN "*~" EXCLUDE
  PATTERN "__pycache__" EXCLUDE
  )