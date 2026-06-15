cmake_minimum_required(VERSION 3.14)
project(sdf)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(OPENGL_VERSION "4.3" CACHE STRING "Force OpenGL 4.3 for compute shaders" FORCE)

include(FetchContent)
FetchContent_Declare(
				raylib
				GIT_REPOSITORY https://github.com/raysan5/raylib.git
				GIT_TAG 6.0
)
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.92.7
)
FetchContent_Declare(
  rlimgui
  GIT_REPOSITORY https://github.com/raylib-extras/rlImGui.git
	GIT_TAG Raylib_6_0
)

FetchContent_MakeAvailable(raylib)
FetchContent_MakeAvailable(rlimgui)
FetchContent_MakeAvailable(imgui)

set( SOURCES 
				main.cpp 
				header/ShaderLogic.hpp header/Controller.hpp header/GUI.hpp 
				shader/lighting.glsl shader/raymarcher.glsl shader/sdfs.glsl )

set( IMGUI 
				${imgui_SOURCE_DIR}/imgui.cpp 
				${imgui_SOURCE_DIR}/imgui_draw.cpp 
				${imgui_SOURCE_DIR}/imgui_tables.cpp 
				${imgui_SOURCE_DIR}/imgui_widgets.cpp )

add_executable(${PROJECT_NAME}
				${SOURCES}
				${IMGUI}
				${rlimgui_SOURCE_DIR}/rlImGui.cpp
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_link_libraries(${PROJECT_NAME} PRIVATE raylib)
target_include_directories(${PROJECT_NAME} PRIVATE
				${rlimgui_SOURCE_DIR}
				${imgui_SOURCE_DIR}
				${CMAKE_CURRENT_SOURCE_DIR}/header
				${CMAKE_CURRENT_SOURCE_DIR}/shader
)
