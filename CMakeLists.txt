cmake_minimum_required(VERSION 3.20)
project(sdf LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_program(GLSLC_EXECUTABLE glslc REQUIRED)

add_executable(sdf src/main.cpp src/camera.cpp src/profiling.cpp
                   src/vulkan/vulkan.cpp src/vulkan/shader.cpp src/vulkan/timing.cpp)
target_compile_features(sdf PRIVATE cxx_std_20)
set_target_properties(sdf PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED YES
                                    CXX_EXTENSIONS NO)
target_compile_definitions(
  sdf
  PRIVATE GLFW_INCLUDE_NONE SDF_SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shader"
          SDF_GLSLC="${GLSLC_EXECUTABLE}")
target_link_libraries(sdf PRIVATE Vulkan::Vulkan glfw)
target_include_directories(sdf PRIVATE src)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(sdf PRIVATE -Wall -Wextra -Wpedantic
                                     -Wno-missing-field-initializers)
endif()
