#include "camera.hpp"

#include <algorithm>

void Camera::Controls(GLFWwindow *window, float deltaTime) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);

  bool tab = glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS;
  bool wasCaptured = captured;
  if (tab && !tabDown)
    captured = !captured;
  if (!glfwGetWindowAttrib(window, GLFW_FOCUSED))
    captured = false;
  tabDown = tab;

  if (captured != wasCaptured) {
    glfwSetInputMode(window, GLFW_CURSOR,
                     captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
  }
  if (!captured)
    return;

  double x, y;
  glfwGetCursorPos(window, &x, &y);
  Math::vec2 cursor{.x = float(x), .y = float(y)};
  if (captured != wasCaptured)
    lastCursor = cursor;
  auto delta = cursor - lastCursor;
  Look(delta.x, delta.y);
  lastCursor = cursor;
  Move(window, deltaTime);
}

void Camera::Look(float deltaX, float deltaY) {
  float yawChange = deltaX * sense * DEG2RAD;
  float nextPitch = std::clamp(pitch - deltaY * sense * float(DEG2RAD),
                               -float(PI / 2), float(PI / 2));
  // World-Y yaw * orientation * local-X pitch. The other axis terms are zero.
  float sy = std::sin(yawChange * 0.5f), cy = std::cos(yawChange * 0.5f);
  float sp = std::sin((pitch - nextPitch) * 0.5f);
  float cp = std::cos((pitch - nextPitch) * 0.5f);
  auto q = orientation;
  Math::quaternion yawed{
      .x = cy * q.x + sy * q.z,
      .y = cy * q.y + sy * q.w,
      .z = cy * q.z - sy * q.x,
      .w = cy * q.w - sy * q.y,
  };
  Math::vec4 rotation{
      .x = cp * yawed.x + sp * yawed.w,
      .y = cp * yawed.y + sp * yawed.z,
      .z = cp * yawed.z - sp * yawed.y,
      .w = cp * yawed.w - sp * yawed.x,
  };

  // Normalize to prevent drift, then use the quaternion's rotation-matrix
  // columns.
  rotation = rotation.normalize();
  orientation = {.x = rotation.x, .y = rotation.y,
                 .z = rotation.z, .w = rotation.w};
  pitch = nextPitch;
  right = {.x = 1 - 2 * (rotation.y * rotation.y + rotation.z * rotation.z),
           .y = 2 * (rotation.x * rotation.y + rotation.w * rotation.z),
           .z = 2 * (rotation.x * rotation.z - rotation.w * rotation.y)};
  up = {.x = 2 * (rotation.x * rotation.y - rotation.w * rotation.z),
        .y = 1 - 2 * (rotation.x * rotation.x + rotation.z * rotation.z),
        .z = 2 * (rotation.y * rotation.z + rotation.w * rotation.x)};
  forward = {.x = 2 * (rotation.x * rotation.z + rotation.w * rotation.y),
             .y = 2 * (rotation.y * rotation.z - rotation.w * rotation.x),
             .z = 1 - 2 * (rotation.x * rotation.x + rotation.y * rotation.y)};
}

void Camera::Move(GLFWwindow *window, float deltaTime) {
  Math::vec3 movement{};
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    movement = movement + forward;
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    movement = movement - forward;
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    movement = movement + right;
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    movement = movement - right;
  if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
    movement.y += 1;
  if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
    movement.y -= 1;

  float distance = speed * deltaTime;
  if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
    distance *= sprint;
  if (movement.length() > 0.0001f)
    pos = pos + movement.normalize() * distance;
}
