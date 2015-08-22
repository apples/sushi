cmake_minimum_required(VERSION 3.2)
project(sushi)

find_package(GLM REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)

add_subdirectory(glad) # glad
add_subdirectory(lodepng) # lodepng

add_library(sushi src/sushi/sushi.hpp src/sushi/window.hpp src/sushi/common.hpp src/sushi/window.cpp src/sushi/key_array.hpp src/sushi/texture.hpp src/sushi/mesh.hpp src/sushi/shader.hpp src/sushi/shader.cpp src/sushi/gl.hpp src/sushi/mesh.cpp src/sushi/texture.cpp)
set_property(TARGET sushi PROPERTY CXX_STANDARD 14)
target_include_directories(sushi PUBLIC src/ ${GLFW_INCLUDE_DIRS} ${GLM_INCLUDE_DIRS})
target_link_libraries(sushi glad lodepng ${GLFW_LIBRARIES} ${GLM_LIBRARIES})

add_executable(sushi_test test/main.cpp)
set_property(TARGET sushi_test PROPERTY CXX_STANDARD 14)
target_link_libraries(sushi_test sushi)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
