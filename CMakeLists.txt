cmake_minimum_required(VERSION 4.0)

project(BlackHole)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 26)

find_package(grf REQUIRED)

add_executable(black_hole
  ${CMAKE_SOURCE_DIR}/src/main.cpp
)

target_compile_definitions(black_hole PRIVATE
  SHADER_DIR="${CMAKE_SOURCE_DIR}/shaders"
)

target_link_libraries(black_hole PRIVATE grf::grf)
