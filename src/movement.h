#pragma once

#include "raylib.h"

struct player {
  Vector3 camPos;

  Vector3 forward;
  Vector3 right;
  Vector3 up;

  float yaw;
  float pitch;
  float speed = 2.0f;
  float fov = 0.75f;
  float lampStr = 200.0f;
  float lampDist = 25.0f;
};

void cameraInitialize(player &cam, float x, float y, float z, float yawIn,
                      float pitchIn);
void cameraMouse(player &cam);
Vector3 cameraMove(player &cam);
