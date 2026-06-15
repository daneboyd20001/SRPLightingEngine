#pragma once

#include <raylib.h>
#include <raymath.h>

class Controller {
public:
  Vector3 pos;

  Quaternion rotation;
  Vector3 forward;
  Vector3 up;
  Vector3 right;

  float speed = 2.0f;
  float sens = 0.002f;
  float fov = 1.570f;
  float lampStr = 200.0f;
  float scalarDist = 1.0f;
  float minDist = 0.001f;

  void Init(float x, float y, float z) {
    pos = {x, y, z};
    rotation = QuaternionIdentity();
    forward = {0.0f, 0.0f, 1.0f};
    up = {0.0f, 1.0f, 0.0f};
    right = {1.0f, 0.0f, 0.0f};
  }

  void UpdateMouse() {
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
      DisableCursor();
    if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT))
      EnableCursor();

    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
      Vector2 mouseDelta = GetMouseDelta();

      Quaternion yaw =
          QuaternionFromAxisAngle({0.0f, 1.0f, 0.0f}, mouseDelta.x * sens);
      Quaternion pitch =
          QuaternionFromAxisAngle({1.0f, 0.0f, 0.0f}, mouseDelta.y * sens);

      // Flip this to get spaceship effect. But, roll gets weird because of
      // it.
      rotation = QuaternionMultiply(yaw, rotation);
      rotation = QuaternionMultiply(rotation, pitch);

      rotation = QuaternionNormalize(rotation);
    }
  }

  Vector3 UpdateMove() {
    forward = Vector3RotateByQuaternion({0.0f, 0.0f, 1.0f}, rotation);
    right = Vector3RotateByQuaternion({1.0f, 0.0f, 0.0f}, rotation);
    up = Vector3RotateByQuaternion({0.0f, 1.0f, 0.0f}, rotation);

    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
      speed = 5.0f * GetFrameTime();
      if (IsKeyDown(KEY_LEFT_SHIFT))
        speed *= 3.5f;

      if (IsKeyDown(KEY_W))
        pos = Vector3Add(pos, Vector3Scale(forward, speed));
      if (IsKeyDown(KEY_S))
        pos = Vector3Subtract(pos, Vector3Scale(forward, speed));
      if (IsKeyDown(KEY_D))
        pos = Vector3Add(pos, Vector3Scale(right, speed));
      if (IsKeyDown(KEY_A))
        pos = Vector3Subtract(pos, Vector3Scale(right, speed));
      if (IsKeyDown(KEY_SPACE))
        pos = Vector3Add(pos, Vector3Scale(up, speed));
      if (IsKeyDown(KEY_C))
        pos = Vector3Subtract(pos, Vector3Scale(up, speed));
    }

    return pos;
  }
  Controller() = default;
  ~Controller() = default;
};
