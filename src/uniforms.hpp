#pragma once

#include "camera.hpp"

#include <cstddef>
#include <cstdint>

// GPU copies of application data, matching shader/common.glsl in field order.
// std140 requires vec3 fields to start on 16-byte boundaries. Padding is
// unused.
struct alignas(16) CameraUniforms {
  float screenSize[4];
  alignas(16) Math::vec3 position;
  alignas(16) Math::vec3 forward;
  alignas(16) Math::vec3 right;
  alignas(16) Math::vec3 up;
  float fovTan; // Shares the last 16-byte slot with up.

  CameraUniforms(const Camera &camera, unsigned width, unsigned height)
      : screenSize{float(width), float(height), 0, 0}, position(camera.pos),
        forward(camera.forward), right(camera.right), up(camera.up),
        fovTan(std::tan(camera.fov * float(DEG2RAD) * 0.5f)) {}
};

struct alignas(16) LightingUniforms {
  float lampStrength;
  std::int32_t activeLighting;
  float padding[2]{};
};

struct alignas(16) SdfUniforms {
  float time;
  float quality;
  float minClip;
  float maxClip;
  std::int32_t activeSDF;
  float padding[3]{};
};

// Catch accidental changes to the CPU/GLSL memory layout at build time.
static_assert(sizeof(CameraUniforms) == 80);
static_assert(offsetof(CameraUniforms, fovTan) == 76);
static_assert(sizeof(LightingUniforms) == 16);
static_assert(sizeof(SdfUniforms) == 32);
static_assert(offsetof(SdfUniforms, activeSDF) == 16);
