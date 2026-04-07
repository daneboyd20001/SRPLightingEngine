#include "shaderLoad.h"

#include <fstream>
#include <sstream>

std::string ReadFile(const std::string &filepath) {
  std::ifstream file(filepath);
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

void load(shader &sh) {
  sh.resolutionLoc = GetShaderLocation(sh.sdfShader, "resolution");
  sh.timeLoc = GetShaderLocation(sh.sdfShader, "time");
  sh.camPosLoc = GetShaderLocation(sh.sdfShader, "camPos");
  sh.forwardLoc = GetShaderLocation(sh.sdfShader, "camForward");
  sh.rightLoc = GetShaderLocation(sh.sdfShader, "camRight");
  sh.upLoc = GetShaderLocation(sh.sdfShader, "camUp");
  sh.activeSDFLoc = GetShaderLocation(sh.sdfShader, "activeSDF");
  sh.activeLightingLoc = GetShaderLocation(sh.sdfShader, "activeLighting");
  sh.lampDistLoc = GetShaderLocation(sh.sdfShader, "lampDist");
  sh.minDistLoc = GetShaderLocation(sh.sdfShader, "MIN_DIST");
  sh.fovLoc = GetShaderLocation(sh.sdfShader, "fov");
  sh.scalarDistLoc = GetShaderLocation(sh.sdfShader, "scalarDist");
  sh.lampStrLoc = GetShaderLocation(sh.sdfShader, "lampStrength");
}

void hotReload(long sdfModTime, long lightingModTime, long mainModTime,
               std::string sdfPath, std::string lightingPath,
               std::string mainPath, shader &sh) {
  long currentSdfModTime = GetFileModTime(sdfPath.c_str());
  long currentLightingModTime = GetFileModTime(lightingPath.c_str());
  long currentMainModTime = GetFileModTime(mainPath.c_str());

  if (currentSdfModTime != sdfModTime ||
      currentLightingModTime != lightingModTime ||
      currentMainModTime != mainModTime) {

    std::string updatedCombinedCode = ReadFile(sdfPath) + "\n" +
                                      ReadFile(lightingPath) + "\n" +
                                      ReadFile(mainPath);
    Shader updatedShader = LoadShaderFromMemory(0, updatedCombinedCode.c_str());

    if (updatedShader.id != rlGetShaderIdDefault()) {
      UnloadShader(sh.sdfShader);
      sh.sdfShader = updatedShader;

      sh.resolutionLoc = GetShaderLocation(sh.sdfShader, "resolution");
      sh.timeLoc = GetShaderLocation(sh.sdfShader, "time");
      sh.camPosLoc = GetShaderLocation(sh.sdfShader, "camPos");
      sh.forwardLoc = GetShaderLocation(sh.sdfShader, "camForward");
      sh.rightLoc = GetShaderLocation(sh.sdfShader, "camRight");
      sh.upLoc = GetShaderLocation(sh.sdfShader, "camUp");
      sh.activeSDFLoc = GetShaderLocation(sh.sdfShader, "activeSDF");
      sh.lampDistLoc = GetShaderLocation(sh.sdfShader, "lampDist");
      sh.activeLightingLoc = GetShaderLocation(sh.sdfShader, "activeLighting");
      sh.minDistLoc = GetShaderLocation(sh.sdfShader, "MIN_DIST");
      sh.fovLoc = GetShaderLocation(sh.sdfShader, "fov");
      sh.scalarDistLoc = GetShaderLocation(sh.sdfShader, "scalarDist");
      sh.lampStrLoc = GetShaderLocation(sh.sdfShader, "lampStrength");
    }
    sdfModTime = currentSdfModTime;
    lightingModTime = currentLightingModTime;
    mainModTime = currentMainModTime;
  }
}

void setShader(shader &sh, player &cam, float minDist, float scalarDist,
               int currentSDF, int currentLight) {
  float time = GetTime();
  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};

  SetShaderValue(sh.sdfShader, sh.resolutionLoc, resolution,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(sh.sdfShader, sh.timeLoc, &time, SHADER_UNIFORM_FLOAT);
  SetShaderValue(sh.sdfShader, sh.camPosLoc, &cam.camPos, SHADER_UNIFORM_VEC3);
  SetShaderValue(sh.sdfShader, sh.forwardLoc, &cam.forward,
                 SHADER_UNIFORM_VEC3);
  SetShaderValue(sh.sdfShader, sh.rightLoc, &cam.right, SHADER_UNIFORM_VEC3);
  SetShaderValue(sh.sdfShader, sh.upLoc, &cam.up, SHADER_UNIFORM_VEC3);
  SetShaderValue(sh.sdfShader, sh.activeSDFLoc, &currentSDF,
                 SHADER_UNIFORM_INT);
  SetShaderValue(sh.sdfShader, sh.lampDistLoc, &cam.lampDist,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(sh.sdfShader, sh.activeLightingLoc, &currentLight,
                 SHADER_UNIFORM_INT);
  SetShaderValue(sh.sdfShader, sh.minDistLoc, &minDist, SHADER_UNIFORM_FLOAT);
  SetShaderValue(sh.sdfShader, sh.fovLoc, &cam.fov, SHADER_UNIFORM_FLOAT);
  SetShaderValue(sh.sdfShader, sh.scalarDistLoc, &scalarDist,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(sh.sdfShader, sh.lampStrLoc, &cam.lampStr,
                 SHADER_UNIFORM_FLOAT);
}
