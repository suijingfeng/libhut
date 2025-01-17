cmake_minimum_required(VERSION 3.6)
project(hut)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra -std=c++1z")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/deps/")

###########################################################
message("Checking for required dependencies...")
###########################################################

find_package(Threads REQUIRED)
set(LIBS ${LIBS} ${CMAKE_THREAD_LIBS_INIT})

find_package(Vulkan REQUIRED)
include_directories(${Vulkan_INCLUDE_DIRS})
set(LIBS ${LIBS} ${Vulkan_LIBRARIES})

find_package(GLM REQUIRED)
include_directories(${GLM_INCLUDE_DIRS})
add_definitions("-DGLM_FORCE_DEPTH_ZERO_TO_ONE -DGLM_FORCE_LEFT_HANDED")
set(LIBS ${LIBS} ${GLM_LIBRARIES})

find_package(PNG REQUIRED)
include_directories(${PNG_INCLUDE_DIRS})
set(LIBS ${LIBS} ${PNG_LIBRARIES})

find_program(GLSLANG_VALIDATOR NAMES glslangValidator)
if (NOT GLSLANG_VALIDATOR)
  message(FATAL_ERROR "Can't find glslangValidator, needed for shader compilation.")
endif ()

###########################################################
message("Checking for optional dependencies...")
###########################################################

find_package(GTest)
include_directories(${GTEST_INCLUDE_DIR})

find_package(XCB COMPONENTS xcb xcb-keysyms)
if (XCB_FOUND AND xcb-keysyms_FOUND)
  message("Using XCB as backend")
  include_directories(${XCB_INCLUDE_DIRS})
  set(LIBS ${LIBS} ${XCB_LIBRARIES})

  file(GLOB HUT_XCB_SOURCES src/xcb/*.cpp)
  set(HUT_SOURCES ${HUT_SOURCES} ${HUT_XCB_SOURCES})
  set(HUT_PLATFORM XCB)
endif ()

if (NOT HUT_PLATFORM)
  message(FATAL_ERROR "Couldn't find a suitable backend")
endif ()

###########################################################
message("Configuring library build targets...")
###########################################################

file(GLOB HUT_SHADER_SOURCES spv/*.frag spv/*.vert)
set(HUT_SHADER_SPV "")
set(HUT_GEN_DIR "${CMAKE_BINARY_DIR}/gen")

file(MAKE_DIRECTORY ${HUT_GEN_DIR}/inc)
file(MAKE_DIRECTORY ${HUT_GEN_DIR}/spv)

FOREACH (shader_source ${HUT_SHADER_SOURCES})
  get_filename_component(shader_target ${shader_source} NAME)
  string(REPLACE ".frag" ".frag.spv" shader_target "${shader_target}")
  string(REPLACE ".vert" ".vert.spv" shader_target "${shader_target}")
  set(shader_target ${HUT_GEN_DIR}/spv/${shader_target})
  set(HUT_SHADER_SPV ${HUT_SHADER_SPV};${shader_target})

  add_custom_command(OUTPUT ${shader_target}
      COMMAND ${GLSLANG_VALIDATOR} -V ${shader_source} -o ${shader_target}
      DEPENDS ${shader_source} ${HUT_GEN_DIR}/spv
      COMMENT "Generating SPIR-V for ${shader_source}")
ENDFOREACH ()

add_custom_target(shaders_spv DEPENDS ${HUT_SHADER_SPV}
    COMMENT "Checked if shaders need recompilation")

add_executable(gen_shaders src/gen_res/main.cpp)
target_link_libraries(gen_shaders -lstdc++fs)

add_custom_command(OUTPUT ${HUT_GEN_DIR}/inc/spv.h
    COMMAND gen_shaders __spv ${HUT_GEN_DIR}/inc/spv.h ${HUT_SHADER_SPV}
    DEPENDS gen_shaders ${HUT_SHADER_SPV}
    COMMENT "Generating spv.h")
include_directories(${HUT_GEN_DIR}/inc)

add_custom_target(gen_shaders_h DEPENDS ${HUT_GEN_DIR}/inc/spv.h
    COMMENT "Checked if spv.h need regeneration")
add_dependencies(gen_shaders_h gen_shaders)

add_definitions(-DVK_USE_PLATFORM_${HUT_PLATFORM}_KHR)
file(GLOB_RECURSE HUT_HEADERS inc/hut/*.hpp inc/hut/drawables/*.hpp ${HUT_GEN_DIR}/inc/spv.h)
file(GLOB HUT_COMMON_SOURCES src/*.cpp)
set(HUT_SOURCES ${HUT_SOURCES} ${HUT_COMMON_SOURCES})

include_directories(inc)
add_library(hut ${HUT_HEADERS} ${HUT_SOURCES})
add_dependencies(hut gen_shaders_h)
target_link_libraries(hut ${LIBS})

###########################################################
message("Configuring demo build targets...")
###########################################################

file(GLOB HUT_DEMO_PNG tst/*.png)
add_custom_command(OUTPUT ${HUT_GEN_DIR}/inc/demo_png.h
    COMMAND gen_shaders demo ${HUT_GEN_DIR}/inc/demo_png.h ${HUT_DEMO_PNG}
    DEPENDS gen_shaders ${HUT_DEMO_PNG}
    COMMENT "Generating demo_png.h")

add_custom_target(gen_demo_h DEPENDS ${HUT_GEN_DIR}/inc/demo_png.h
    COMMENT "Checked if demo_png.h need regeneration")

add_executable(demo tst/demo.cpp)
target_link_libraries(demo hut)
add_dependencies(demo gen_demo_h)

if (GTEST_FOUND)
  ###########################################################
  message("Enabling tests...")
  ###########################################################

  enable_testing()

  file(GLOB HUT_UNIT_TESTS tst/unit/*.cpp)
  add_executable(unittests ${HUT_UNIT_TESTS})
  add_test(NAME unittests WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} COMMAND unittests)
  target_link_libraries(unittests hut ${GTEST_BOTH_LIBRARIES})
endif ()
