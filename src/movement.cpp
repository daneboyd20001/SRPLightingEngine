#include "movement.h"
#include "raymath.h"

void cameraInitialize(player &cam, float x, float y, float z) {
  cam.camPos = {x, y, z};

  cam.rotation = QuaternionIdentity();

  cam.forward = {0.0f, 0.0f, 1.0f};
  cam.up = {0.0f, 1.0f, 0.0f};
  cam.right = {1.0f, 0.0f, 0.0f};
}

void cameraMouse(player &cam) {
  if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
    DisableCursor();
  if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT))
    EnableCursor();

  if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
    Vector2 mouseDelta = GetMouseDelta();

    Quaternion yaw = QuaternionFromAxisAngle({0.0f, 1.0f, 0.0f}, mouseDelta.x * 0.002f);
    Quaternion pitch = QuaternionFromAxisAngle({1.0f, 0.0f, 0.0f}, mouseDelta.y * 0.002f);

    cam.rotation = QuaternionMultiply(yaw, cam.rotation); // Flip this to get spaceship effect. But, roll gets weird because of it.
    cam.rotation = QuaternionMultiply(cam.rotation, pitch);

    cam.rotation = QuaternionNormalize(cam.rotation);
  }
}

Vector3 cameraMove(player &cam) {
  cam.forward = Vector3RotateByQuaternion({0.0f, 0.0f, 1.0f}, cam.rotation);
  cam.right = Vector3RotateByQuaternion({1.0f, 0.0f, 0.0f}, cam.rotation);
  cam.up = Vector3RotateByQuaternion({0.0f, 1.0f, 0.0f}, cam.rotation);

  if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
    cam.speed = 5.0f * GetFrameTime();
    if (IsKeyDown(KEY_LEFT_SHIFT))
      cam.speed *= 3.5f;

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
