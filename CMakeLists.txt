cmake_minimum_required(VERSION 3.14)
project(sdfengine)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
FetchContent_Declare(
    raylib
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz
  )
FetchContent_GetProperties(raylib)
if(NOT raylib_POPULATED)
  set(FETCHCONTENT_QUIET NO)
  set(OPENGL_VERSION "4.3" CACHE STRING "OpenGL version" FORCE)
  FetchContent_MakeAvailable(raylib)
  set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
endif()
include(FetchContent)

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG master
)
FetchContent_MakeAvailable(imgui)

FetchContent_Declare(
  rlimgui
  GIT_REPOSITORY https://github.com/raylib-extras/rlImGui.git
  GIT_TAG main
)
FetchContent_MakeAvailable(rlimgui)

add_executable(${PROJECT_NAME}
    src/core/main.cpp
    src/ShaderLoading/shaderLoad.cpp
    src/core/movement.cpp
    src/GUI/gui.cpp
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${rlimgui_SOURCE_DIR}/rlImGui.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE raylib)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${imgui_SOURCE_DIR}
    ${rlimgui_SOURCE_DIR}
)
