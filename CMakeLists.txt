cmake_minimum_required(VERSION 3.14...4.3)
project(sdf)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(OPENGL_VERSION
    "4.3"
    CACHE STRING "Force OpenGL 4.3 for Compute Shaders" FORCE)

include(FetchContent)
FetchContent_Declare(
  raylib
  GIT_REPOSITORY https://github.com/raysan5/raylib.git
  GIT_TAG 6.0)
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG docking)
FetchContent_Declare(
  rlimgui
  GIT_REPOSITORY https://github.com/raylib-extras/rlImGui.git
  GIT_TAG Raylib_6_0)

FetchContent_MakeAvailable(raylib)
FetchContent_MakeAvailable(imgui)
FetchContent_MakeAvailable(rlImGui)

set(SOURCES main.cpp include/shader-logic.hpp include/gui.hpp)

set(IMGUI
    ${imgui_SOURCE_DIR}/imgui.cpp ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp ${imgui_SOURCE_DIR}/imgui_widgets.cpp)

add_executable(${PROJECT_NAME} ${SOURCES} ${IMGUI}
                               ${rlimgui_SOURCE_DIR}/rlImGui.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE raylib)
target_include_directories(
  ${PROJECT_NAME} PRIVATE ${rlimgui_SOURCE_DIR} ${imgui_SOURCE_DIR}
                          ${CMAKE_CURRENT_SOURCE_DIR}/include)
