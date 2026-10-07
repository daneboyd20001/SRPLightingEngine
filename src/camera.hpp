#pragma once

#include <GLFW/glfw3.h>

#include "math.hpp"

class Camera {
private:
  bool captured = false;
  bool tabDown = false;
  Math::vec2 lastCursor{};
  float pitch = 0.0f;
  Math::quaternion orientation;

  void Look(float deltaX, float deltaY);
  void Move(GLFWwindow *window, float deltaTime);

public:
  Math::vec3 pos{0.0f, 0.0f, -3.0f}, forward{0.0f, 0.0f, 1.0f},
      right{1.0f, 0.0f, 0.0f}, up{0.0f, 1.0f, 0.0f};
  float speed{10.0f}, fov{90.0f}, sprint{1.5f}, sense{0.1f};

  void Controls(GLFWwindow *window, float deltaTime);
};
