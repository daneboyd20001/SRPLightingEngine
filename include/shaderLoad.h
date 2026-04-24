#pragma once

#include "movement.h"
#include "rlgl.h"

#include <string>

using namespace std;

struct shader {
  unsigned int computeProgramId;

  int camPosLoc;
  int camToWorldLoc;
  int screenSizeLoc;
  int timeLoc;
  int activeSDFLoc;
  int activeLightingLoc;
  int lampDistLoc;
  int lampStrengthLoc;
  int fovLoc;
  int scalarDistLoc;
  int minDistLoc;

  void load();
  void hotReload();
  void setShader(player &cam, int activeSDF, int activeLighting,
                 float scalarDist, float minDist);
};

string ReadFile(const string &filepath);
