cmake_minimum_required(VERSION 3.2)
project(sushi)

include(FetchContent)

set(SUSHI_BUILD_EXAMPLES No CACHE BOOL "Build Sushi examples")

if(NOT TARGET glm)
    find_package(glm REQUIRED)
endif()

if(NOT TARGET lodepng)
    FetchContent_Declare(
        lodepng
        GIT_REPOSITORY https://github.com/apples/lodepng.git
        GIT_TAG        cmake)
    FetchContent_GetProperties(lodepng)
    if(NOT lodepng_POPULATED)
        FetchContent_Populate(lodepng)
        add_subdirectory(${lodepng_SOURCE_DIR} ${lodepng_BINARY_DIR})
    endif()
endif()

add_library(sushi
    src/sushi/sushi.hpp
    src/sushi/attrib_location.hpp
    src/sushi/common.hpp
    src/sushi/gl.hpp
    src/sushi/gles_shim.hpp
    src/sushi/texture.hpp src/sushi/texture.cpp
    src/sushi/mesh_utils.hpp
    src/sushi/mesh_group.hpp src/sushi/mesh_group.cpp
    src/sushi/skeleton.hpp src/sushi/skeleton.cpp
    src/sushi/pose.hpp src/sushi/pose.cpp
    src/sushi/mesh_builder.hpp src/sushi/mesh_builder.cpp
    src/sushi/obj_loader.hpp src/sushi/obj_loader.cpp
    src/sushi/shader.hpp src/sushi/shader.cpp
    src/sushi/iqm.hpp src/sushi/iqm.cpp
    src/sushi/framebuffer.cpp src/sushi/framebuffer.hpp
    src/sushi/framebuffer_cubemap.cpp src/sushi/framebuffer_cubemap.hpp
    src/sushi/frustum.cpp src/sushi/frustum.hpp
    src/sushi/transform.cpp src/sushi/transform.hpp
)
set_target_properties(sushi PROPERTIES CXX_STANDARD 17)
target_include_directories(sushi PUBLIC src/)
target_link_libraries(sushi glm lodepng)

if(NOT EMSCRIPTEN)
    add_subdirectory(ext/glad)
    target_link_libraries(sushi glad)
endif()

if(SUSHI_BUILD_EXAMPLES)
    find_package(glfw3 REQUIRED)
    add_executable(sushi_test test/main.cpp)
    set_target_properties(sushi_test PROPERTIES CXX_STANDARD 17)
    target_link_libraries(sushi_test sushi glfw)
endif()
