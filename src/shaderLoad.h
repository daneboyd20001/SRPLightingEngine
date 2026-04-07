#pragma once

#include "movement.h"
#include "raylib.h"
#include "rlgl.h"

#include <string>

struct shader {
  Shader sdfShader;

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
};

std::string ReadFile(const std::string &filepath);
void load(shader &sh);
void hotReload(long sdfModTime, long lightingModTime, long mainModTime,
               std::string sdfPath, std::string lightingPath,
               std::string mainPath, shader &sh);
void setShader(shader &sh, player &cam, float minDist, float scalarDist,
               int currentSDF, int currentLight);
