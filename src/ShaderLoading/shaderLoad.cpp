#include "shaderLoad.h"
#include "raymath.h"
#include "rlgl.h"
#include "shaderLoad.h"

#include <fstream>
#include <sstream>

string ReadFile(const string &filepath) {
  ifstream file(filepath);
  stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

void shader::load() {
  string noisePath = "../shaders/noise.glsl";
  string sdfPath = "../shaders/sdfs.glsl";
  string lightingPath = "../shaders/lighting.glsl";
  string mainPath = "../shaders/raymarcher.glsl";
  string versionHeader = "#version 430 core\n";

  string combinedShaderCode =
      versionHeader + ReadFile(noisePath) + "\n" + ReadFile(sdfPath) + "\n" +
      ReadFile(lightingPath) + "\n" + ReadFile(mainPath);

  unsigned int compId =
      rlCompileShader(combinedShaderCode.c_str(), RL_COMPUTE_SHADER);
  computeProgramId = rlLoadComputeShaderProgram(compId);

  camPosLoc = rlGetLocationUniform(computeProgramId, "camPos");
  camToWorldLoc = rlGetLocationUniform(computeProgramId, "CamToWorld");
  screenSizeLoc = rlGetLocationUniform(computeProgramId, "ScreenSize");
  timeLoc = rlGetLocationUniform(computeProgramId, "time");
  activeSDFLoc = rlGetLocationUniform(computeProgramId, "activeSDF");
  activeLightingLoc = rlGetLocationUniform(computeProgramId, "activeLighting");
  lampDistLoc = rlGetLocationUniform(computeProgramId, "lampDist");
  lampStrengthLoc = rlGetLocationUniform(computeProgramId, "lampStrength");
  fovLoc = rlGetLocationUniform(computeProgramId, "fov");
  scalarDistLoc = rlGetLocationUniform(computeProgramId, "scalarDist");
  minDistLoc = rlGetLocationUniform(computeProgramId, "minDist");
}

void shader::hotReload() {
  if (IsKeyPressed(KEY_R)) {
    rlUnloadShaderProgram(computeProgramId);
    load();
  }
}

void shader::setShader(player &cam, int activeSDF, int activeLighting,
                       float scalarDist, float minDist) {
  rlEnableShader(computeProgramId);

  float pos[3] = {cam.camPos.x, cam.camPos.y, cam.camPos.z};
  rlSetUniform(camPosLoc, pos, RL_SHADER_UNIFORM_VEC3, 1);

  float screen[4] = {(float)GetScreenWidth(), (float)GetScreenHeight(),
                     (float)GetScreenWidth() / (float)GetScreenHeight(), 1.0f};
  rlSetUniform(screenSizeLoc, screen, RL_SHADER_UNIFORM_VEC4, 1);

  Matrix camMatrix = QuaternionToMatrix(cam.rotation);
  rlSetUniformMatrix(camToWorldLoc, camMatrix);
  float time = (float)GetTime();
  rlSetUniform(timeLoc, &time, RL_SHADER_UNIFORM_FLOAT, 1);
  rlSetUniform(fovLoc, &cam.fov, RL_SHADER_UNIFORM_FLOAT, 1);
  rlSetUniform(lampDistLoc, &cam.lampDist, RL_SHADER_UNIFORM_FLOAT, 1);
  rlSetUniform(lampStrengthLoc, &cam.lampStr, RL_SHADER_UNIFORM_FLOAT, 1);
  rlSetUniform(activeSDFLoc, &activeSDF, RL_SHADER_UNIFORM_INT, 1);
  rlSetUniform(activeLightingLoc, &activeLighting, RL_SHADER_UNIFORM_INT, 1);
  rlSetUniform(scalarDistLoc, &scalarDist, RL_SHADER_UNIFORM_FLOAT, 1);
  rlSetUniform(minDistLoc, &minDist, RL_SHADER_UNIFORM_FLOAT, 1);
}
