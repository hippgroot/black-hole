cmake_minimum_required(VERSION 4.0)

project(BlackHole)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 26)

find_package(grf REQUIRED)

add_executable(black_hole
  ${CMAKE_SOURCE_DIR}/include/camera.hpp

  ${CMAKE_SOURCE_DIR}/src/camera.cpp
  ${CMAKE_SOURCE_DIR}/src/main.cpp
)

target_include_directories(black_hole PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_compile_definitions(black_hole PRIVATE
  SHADER_DIR="${CMAKE_SOURCE_DIR}/shaders"
  ASSET_DIR="${CMAKE_SOURCE_DIR}/assets"
)

target_link_libraries(black_hole PRIVATE grf::grf)
