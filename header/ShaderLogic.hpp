#pragma once

#include "Controller.hpp"
#include <raylib.h>
#include <rlgl.h>

#include <fstream>
#include <sstream>
#include <string>

using namespace std;

class ShaderLogic {
public:
  ShaderLogic() = default;
  ShaderLogic(const ShaderLogic &) = delete;
  ShaderLogic &operator=(const ShaderLogic &) = delete;
  ~ShaderLogic() {
    if (computeProgram != 0) {
      rlUnloadShaderProgram(computeProgram);
    }
    if (computeShader != 0) {
      rlUnloadShader(computeShader);
    }
  }

  unsigned int computeShader = 0;
  unsigned int computeProgram = 0;

  int resolutionLoc;
  int timeLoc;
  int camPosLoc;
  int forwardLoc;
  int rightLoc;
  int upLoc;
  int activeSDFLoc;
  int activeLightingLoc;
  int lampDistLoc;
  int minDistLoc;
  int fovLoc;
  int scalarDistLoc;
  int lampStrLoc;

  string ReadFile(const string &filepath) {
    ifstream file(filepath);
    stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
  }

  void load() {
    resolutionLoc = rlGetLocationUniform(computeProgram, "ScreenSize");
    timeLoc = rlGetLocationUniform(computeProgram, "time");
    camPosLoc = rlGetLocationUniform(computeProgram, "camPos");
    forwardLoc = rlGetLocationUniform(computeProgram, "camForward");
    rightLoc = rlGetLocationUniform(computeProgram, "camRight");
    upLoc = rlGetLocationUniform(computeProgram, "camUp");
    activeSDFLoc = rlGetLocationUniform(computeProgram, "activeSDF");
    activeLightingLoc = rlGetLocationUniform(computeProgram, "activeLighting");
    lampDistLoc = rlGetLocationUniform(computeProgram, "lampDist");
    minDistLoc = rlGetLocationUniform(computeProgram, "minDist");
    fovLoc = rlGetLocationUniform(computeProgram, "FOV_Tan");
    scalarDistLoc = rlGetLocationUniform(computeProgram, "scalarDist");
    lampStrLoc = rlGetLocationUniform(computeProgram, "lampStrength");
  }

  void setShader(Controller &cam, int currentSDF, int currentLight) {
    float resolution[4] = {(float)GetScreenWidth(), (float)GetScreenHeight(),
                           0.0f, 0.0f};
    float time = (float)GetTime();
    float fovTan = tan(cam.fov * 0.5f);

    rlSetUniform(resolutionLoc, resolution, SHADER_UNIFORM_VEC4, 1);
    rlSetUniform(timeLoc, &time, SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(camPosLoc, &cam.pos, SHADER_UNIFORM_VEC3, 1);
    rlSetUniform(forwardLoc, &cam.forward, SHADER_UNIFORM_VEC3, 1);
    rlSetUniform(rightLoc, &cam.right, SHADER_UNIFORM_VEC3, 1);
    rlSetUniform(upLoc, &cam.up, SHADER_UNIFORM_VEC3, 1);
    rlSetUniform(activeSDFLoc, &currentSDF, SHADER_UNIFORM_INT, 1);
    rlSetUniform(activeLightingLoc, &currentLight, SHADER_UNIFORM_INT, 1);
    rlSetUniform(minDistLoc, &cam.minDist, SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(scalarDistLoc, &cam.scalarDist, SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(fovLoc, &fovTan, SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(lampStrLoc, &cam.lampStr, SHADER_UNIFORM_FLOAT, 1);
  }
};
