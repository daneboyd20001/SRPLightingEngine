#pragma once

#include <GLFW/glfw3.h>
#include <stdexcept>

// Owns this application's single window and GLFW lifetime.
class Window {
public:
  GLFWwindow *handle = nullptr;

  Window(int width, int height, const char *title) {
    if (!glfwInit())
      throw std::runtime_error("GLFW initialization failed");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    handle = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!handle) {
      glfwTerminate();
      throw std::runtime_error("Window creation failed");
    }
  }

  ~Window() {
    glfwDestroyWindow(handle);
    glfwTerminate();
  }
  Window(const Window &) = delete;
  Window &operator=(const Window &) = delete;
};
