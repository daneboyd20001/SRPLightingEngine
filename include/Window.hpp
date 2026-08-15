#pragma once

#include <glad/glad.h>
// ---------------------- //
#include <GLFW/glfw3.h>
#include <iostream>

class Window {
private:
  void static framebuffer_size_callback(GLFWwindow *window, int width,
                                        int height) {
    glViewport(0, 0, width, height);
  }

public:
  GLFWwindow *window{nullptr};

  Window() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(1280, 720, "SDF", NULL, NULL);
    if (window == NULL) {
      std::cout << "Failed to create GLFW window\n";
      glfwTerminate();
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
      std::cout << "Failed to initialize OpenGL context\n";
    }
  }
  ~Window() { glfwTerminate(); }
};
