cmake_minimum_required(VERSION 3.2)
project(sushi)

add_subdirectory(glm) # glm
add_subdirectory(glfw) # glfw
add_subdirectory(glad) # glad
add_subdirectory(lodepng) # lodepng

add_library(sushi
    src/sushi/sushi.hpp
    src/sushi/common.hpp
    src/sushi/gl.hpp
    src/sushi/key_array.hpp
    src/sushi/event.hpp
    src/sushi/window.hpp src/sushi/window.cpp
    src/sushi/texture.hpp src/sushi/texture.cpp
    src/sushi/mesh.hpp src/sushi/mesh.cpp
    src/sushi/shader.hpp src/sushi/shader.cpp
    src/sushi/iqm.hpp src/sushi/iqm.cpp
    src/sushi/framebuffer.cpp src/sushi/framebuffer.hpp
    src/sushi/framebuffer_cubemap.cpp src/sushi/framebuffer_cubemap.hpp
)
set_property(TARGET sushi PROPERTY CXX_STANDARD 14)
target_include_directories(sushi PUBLIC src/)
target_link_libraries(sushi glad lodepng glm glfw)
target_include_directories(sushi PUBLIC glfw/include)

add_executable(sushi_test test/main.cpp)
set_property(TARGET sushi_test PROPERTY CXX_STANDARD 14)
target_link_libraries(sushi_test sushi)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
