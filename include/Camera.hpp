#pragma once

#include <GLFW/glfw3.h>
#include <Math.hpp>

class Camera {
private:
public:
  Camera() = default;
  ~Camera() = default;

  Math::vec3 pos{0.0f, 0.0f, -3.0f}, forward{0.0f, 0.0f, 1.0f},
      right{1.0f, 0.0f, 0.0f}, up{0.0f, 1.0f, 0.0f};
  float speed{10.0f}, fov{90.0f}, sprint{1.5f}, sense{0.1f};

  void Controlles(GLFWwindow *window, float deltaTime) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
      glfwSetWindowShouldClose(window, true);
    Math::vec3 move{};

    float currSpeed = speed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT))
      currSpeed *= sprint;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
      move.z += currSpeed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
      move.z -= currSpeed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
      move.x += currSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
      move.x -= currSpeed;

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
      move.y += currSpeed;
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
      move.y -= currSpeed;

    pos = pos + move;
    // TODO : Mouse controlles go here (use quaternions nerd)
  }
};
