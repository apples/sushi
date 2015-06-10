cmake_minimum_required(VERSION 3.2)
project(sushi)

find_package(GLM REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)

add_subdirectory(glad) # glad
add_subdirectory(lodepng) # lodepng

set(SOURCE_FILES test/main.cpp)
add_executable(sushi ${SOURCE_FILES})
set_property(TARGET sushi PROPERTY CXX_STANDARD 14)
target_include_directories(sushi PUBLIC ${GLFW_INCLUDE_DIRS} ${GLM_INCLUDE_DIRS})
target_link_libraries(sushi glad lodepng ${GLFW_LIBRARIES} ${GLM_LIBRARIES})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
