

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

  compiler/rendermodule.cpp
  compiler/types.cpp
  compiler/errors.cpp
  compiler/symboltable.cpp
  compiler/operations.cpp
  compiler/llvm_helper.cpp
  compiler/gen_state.cpp
  compiler/lookup.cpp
  compiler/value.cpp

  compiler/builtin_ops.cpp  
  compiler/gd_std.cpp

  compiler/types/primitive.cpp
  compiler/types/special.cpp
  compiler/types/vector.cpp

  compiler/ast/ast.cpp
  compiler/ast/node.cpp
  compiler/ast/literal.cpp
  compiler/ast/expression.cpp
  compiler/ast/variable.cpp
  compiler/ast/function.cpp
  compiler/ast/statement.cpp
  compiler/ast/control.cpp
  compiler/ast/distribution.cpp
  compiler/ast/module.cpp
  compiler/ast/alias.cpp
  )

find_package(BISON)
find_package(FLEX)
bison_target(rtl_parser compiler/language/rtlparser.y ${CMAKE_CURRENT_BINARY_DIR}/rtlparser.cpp)
flex_target(rtl_scanner compiler/language/rtlscanner.l ${CMAKE_CURRENT_BINARY_DIR}/rtlscanner.cpp COMPILE_FLAGS "--header-file=${CMAKE_CURRENT_BINARY_DIR}/rtlscanner.hpp" )
add_flex_bison_dependency(rtl_scanner rtl_parser)

set(RT_PARSER_LEXER_SOURCE ${BISON_rtl_parser_OUTPUTS} ${FLEX_rtl_scanner_OUTPUTS})

set(LLVM_INCLUDE_DIR /usr/local/include)
set(LLVM_DEFINITIONS -D_DEBUG -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS)
set(LLVM_LIB_DIR /usr/local/lib)
set(LLVM_LIBS 
  LLVMAsmParser LLVMInstrumentation LLVMLinker LLVMArchive LLVMBitReader LLVMDebugInfo LLVMJIT LLVMipo LLVMVectorize LLVMBitWriter LLVMTableGen LLVMHexagonCodeGen LLVMHexagonAsmPrinter LLVMHexagonDesc LLVMHexagonInfo LLVMNVPTXCodeGen LLVMNVPTXDesc LLVMNVPTXInfo LLVMNVPTXAsmPrinter LLVMMBlazeDisassembler LLVMMBlazeAsmParser LLVMMBlazeCodeGen LLVMMBlazeDesc LLVMMBlazeInfo LLVMMBlazeAsmPrinter LLVMCppBackendCodeGen LLVMCppBackendInfo LLVMMSP430CodeGen LLVMMSP430Desc LLVMMSP430AsmPrinter LLVMMSP430Info LLVMXCoreCodeGen LLVMXCoreDesc LLVMXCoreInfo LLVMCellSPUCodeGen LLVMCellSPUDesc LLVMCellSPUInfo LLVMMipsDisassembler LLVMMipsCodeGen LLVMMipsAsmParser LLVMMipsDesc LLVMMipsInfo LLVMMipsAsmPrinter LLVMARMDisassembler LLVMARMAsmParser LLVMARMCodeGen LLVMARMDesc LLVMARMInfo LLVMARMAsmPrinter LLVMPowerPCCodeGen LLVMPowerPCDesc LLVMPowerPCInfo LLVMPowerPCAsmPrinter LLVMSparcCodeGen LLVMSparcDesc LLVMSparcInfo LLVMX86AsmParser LLVMX86Disassembler LLVMX86CodeGen LLVMSelectionDAG LLVMAsmPrinter LLVMX86Desc LLVMX86Info LLVMX86AsmPrinter LLVMX86Utils LLVMMCDisassembler LLVMMCParser LLVMInterpreter LLVMCodeGen LLVMScalarOpts LLVMInstCombine LLVMTransformUtils LLVMipa LLVMAnalysis LLVMMCJIT LLVMRuntimeDyld LLVMExecutionEngine LLVMTarget LLVMMC LLVMObject LLVMCore LLVMSupport
)

include_directories(${RT_INCLUDE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
link_directories(${LLVM_LIB_DIR})
add_definitions(${LLVM_DEFINITIONS})

#add_library(raytrace SHARED ${RT_SOURCE_FILES} ${RT_PARSER_LEXER_SOURCE})
add_executable(raytrace ${RT_SOURCE_FILES} ${RT_PARSER_LEXER_SOURCE} ${CMAKE_SOURCE_DIR}/tests/test_main.cpp)

target_link_libraries(raytrace ${LLVM_LIBS} pthread m dl)

set_target_properties(raytrace
  PROPERTIES
  COMPILE_FLAGS "-std=c++0x"
  )