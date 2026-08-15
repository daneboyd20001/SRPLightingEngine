#pragma once

#include <glad/glad.h>
// ----------------------- /
#include <Camera.hpp>
#include <GLFW/glfw3.h>
#include <bits/stdc++.h>
#include <fstream>
#include <sstream>

class Shader {
private:
  GLint uScreenSize, uCamPos, uCamForward, uCamRight, uCamUp;
  GLint uTime, uLampDist, uLampStrength, uFovTan, uMinDist, uScalarDist,
      uQuality, uMinClip, uMaxClip;
  GLint uActiveSDF, uActiveLighting;

public:
  GLuint compID{}, screenTex{};
  int texWidth{}, texHeight{};
  float minDist{0.01f}, scalarDist{0.5f}, lampDist{5.0f}, lampStr{100.0f},
      quality{1.0f}, minClip{0.001f}, maxClip{100.0f};

  Shader(const char *cPath) {
    std::string cCode;
    std::ifstream cShaderFile;

    cShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
      cShaderFile.open(cPath);

      std::stringstream cShaderStream, vShaderStream, fShaderStream;
      cShaderStream << cShaderFile.rdbuf();

      cShaderFile.close();

      cCode = cShaderStream.str();

    } catch (std::ifstream::failure e) {
      std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ\n";
    }

    const char *compute = cCode.c_str();

    GLuint shader = glCreateShader(GL_COMPUTE_SHADER);

    GLint success;
    char infoLog[512];

    glShaderSource(shader, 1, &compute, NULL);
    glCompileShader(shader);

    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
      glGetShaderInfoLog(shader, 512, NULL, infoLog);
      std::cout << "ERROR::SHADER::COMPUTE::COMPILATION_FAILED\n"
                << infoLog << "\n";
    }

    compID = glCreateProgram();
    glAttachShader(compID, shader);
    glLinkProgram(compID);
    glGetProgramiv(compID, GL_LINK_STATUS, &success);
    if (!success) {
      glGetProgramInfoLog(compID, 512, NULL, infoLog);
      std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
                << infoLog << "\n";
    }

    glDeleteShader(shader);

    uScreenSize = glGetUniformLocation(compID, "ScreenSize");
    uCamPos = glGetUniformLocation(compID, "camPos");
    uCamForward = glGetUniformLocation(compID, "camForward");
    uCamRight = glGetUniformLocation(compID, "camRight");
    uCamUp = glGetUniformLocation(compID, "camUp");
    uTime = glGetUniformLocation(compID, "time");
    uLampDist = glGetUniformLocation(compID, "lampDist");
    uLampStrength = glGetUniformLocation(compID, "lampStrength");
    uFovTan = glGetUniformLocation(compID, "FOV_Tan");
    uMinDist = glGetUniformLocation(compID, "minDist");
    uScalarDist = glGetUniformLocation(compID, "scalarDist");
    uQuality = glGetUniformLocation(compID, "quality");
    uMinClip = glGetUniformLocation(compID, "MinClip");
    uMaxClip = glGetUniformLocation(compID, "MaxClip");
    uActiveSDF = glGetUniformLocation(compID, "activeSDF");
    uActiveLighting = glGetUniformLocation(compID, "activeLighting");

    // NOTE : Create texture to display compute shader output on.
    glCreateTextures(GL_TEXTURE_2D, 1, &screenTex);
    glTextureParameteri(screenTex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(screenTex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(screenTex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(screenTex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindImageTexture(0, screenTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
  }
  ~Shader() { glDeleteProgram(compID); }

  void resizeCompute(int width, int height) {
    if (width <= 0 || height <= 0)
      return;
    if (width == texWidth && height == texHeight)
      return;

    if (screenTex != 0)
      glDeleteTextures(1, &screenTex);

    glCreateTextures(GL_TEXTURE_2D, 1, &screenTex);
    glTextureParameteri(screenTex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(screenTex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(screenTex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(screenTex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureStorage2D(screenTex, 1, GL_RGBA32F, width, height);
    glBindImageTexture(0, screenTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    texWidth = width;
    texHeight = height;
  }

  void use(const Camera &camera, float time, float fovTan, int activeSDF,
           int activeLighting) {
    if (texWidth <= 0 || texHeight <= 0)
      return;

    glUseProgram(compID);

    glUniform4f(uScreenSize, (float)texWidth, (float)texHeight, 1.0f, 1.0f);

    glUniform3f(uCamPos, camera.pos.x, camera.pos.y, camera.pos.z);
    glUniform3f(uCamForward, camera.forward.x, camera.forward.y,
                camera.forward.z);
    glUniform3f(uCamRight, camera.right.x, camera.right.y, camera.right.z);
    glUniform3f(uCamUp, camera.up.x, camera.up.y, camera.up.z);

    glUniform1f(uTime, time);
    glUniform1f(uLampDist, lampDist);
    glUniform1f(uLampStrength, lampStr);
    glUniform1f(uFovTan, fovTan);
    glUniform1f(uMinDist, minDist);
    glUniform1f(uScalarDist, scalarDist);
    glUniform1f(uQuality, quality);
    glUniform1f(uMinClip, minClip);
    glUniform1f(uMaxClip, maxClip);

    glUniform1i(uActiveSDF, activeSDF);
    glUniform1i(uActiveLighting, activeLighting);
    glBindImageTexture(0, screenTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    glDispatchCompute(ceil(texWidth / 8.0f), ceil(texHeight / 8.0f), 1);
    glMemoryBarrier(
        GL_ALL_BARRIER_BITS); // NOTE : This is a general barrier, there are
                              // other more specific ones if needed
  }
};
