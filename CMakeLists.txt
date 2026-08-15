cmake_minimum_required(VERSION 3.14)
project(sdf)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(OPENGL_VERSION
    "4.3"
    CACHE STRING "Force OpenGL 4.3 for Compute Shaders" FORCE)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

include(FetchContent)
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG docking)
FetchContent_MakeAvailable(imgui)

set(IMGUI
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp)

add_executable(${PROJECT_NAME} main.cpp src/glad.c ${IMGUI})

target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL glfw)
target_include_directories(
  ${PROJECT_NAME}
  PRIVATE ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends
          ${CMAKE_CURRENT_SOURCE_DIR}/include
          ${CMAKE_CURRENT_SOURCE_DIR}/include/external)
