#include "movement.h"
#include "raymath.h"

void cameraInitialize(player &cam, float x, float y, float z, float yawIn,
                      float pitchIn) {
  cam.camPos = {x, y, z};
  cam.yaw = PI / yawIn;
  cam.pitch = pitchIn;
}

void cameraMouse(player &cam) {
  if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
    DisableCursor();
  if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT))
    EnableCursor();

  if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
    Vector2 mouseDelta = GetMouseDelta();
    cam.yaw += mouseDelta.x * 0.002f;
    cam.pitch -= mouseDelta.y * 0.002f;
    cam.pitch = Clamp(cam.pitch, -1.5f, 1.5f);
  }
}

Vector3 cameraMove(player &cam) {
  cam.forward = {cosf(cam.pitch) * cosf(cam.yaw), sinf(cam.pitch),
                 cosf(cam.pitch) * sinf(cam.yaw)};
  cam.right =
      Vector3Normalize(Vector3CrossProduct(cam.forward, {0.0f, 1.0f, 0.0f}));
  cam.up = Vector3CrossProduct(cam.right, cam.forward);

  if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
    cam.speed = 5.0f * GetFrameTime();
    if (IsKeyDown(KEY_LEFT_SHIFT))
      cam.speed *= 2.5f;

    if (IsKeyDown(KEY_W))
      cam.camPos = Vector3Add(cam.camPos, Vector3Scale(cam.forward, cam.speed));
    if (IsKeyDown(KEY_S))
      cam.camPos =
          Vector3Subtract(cam.camPos, Vector3Scale(cam.forward, cam.speed));
    if (IsKeyDown(KEY_D))
      cam.camPos = Vector3Add(cam.camPos, Vector3Scale(cam.right, cam.speed));
    if (IsKeyDown(KEY_A))
      cam.camPos =
          Vector3Subtract(cam.camPos, Vector3Scale(cam.right, cam.speed));
    if (IsKeyDown(KEY_SPACE))
      cam.camPos = Vector3Add(cam.camPos, Vector3Scale(cam.up, cam.speed));
    if (IsKeyDown(KEY_C))
      cam.camPos = Vector3Subtract(cam.camPos, Vector3Scale(cam.up, cam.speed));
  }

  return cam.camPos;
}
