#pragma once

#include "shader.hpp"
#include "../profiling.hpp"

#include <vulkan/vulkan.h>

#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

struct GLFWwindow;

namespace Vulkan {

struct BufferBinding {
  std::uint32_t binding;
  std::size_t bytes;
};

struct RendererConfig {
  VkFormat outputFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
  std::uint32_t outputBinding = 0;
  std::vector<BufferBinding> uniforms;
  bool flipOutputY = false;
  bool vsync = true;
  bool validation = false;
};

struct BufferUpload {
  std::uint32_t binding;
  std::span<const std::byte> bytes;
};

template <typename T>
BufferUpload Upload(std::uint32_t binding, const T &data) {
  static_assert(std::is_trivially_copyable_v<T>);
  return {.binding = binding, .bytes = std::as_bytes(std::span{&data, 1})};
}

class Renderer {
public:
  Renderer(GLFWwindow *window, const RendererConfig &config,
           const ShaderBinary &shader);
  ~Renderer();
  Renderer(const Renderer &) = delete;
  Renderer &operator=(const Renderer &) = delete;

  // Waits for this frame slot and acquires an image. False while minimized or
  // after an out-of-date acquisition. Query Extent after a successful call.
  bool BeginFrame(std::uint64_t frameNumber);
  VkExtent2D Extent() const;
  void Submit(std::span<const BufferUpload> uploads,
              std::array<std::uint32_t, 3> groups);
  bool ReplaceShader(const ShaderBinary &shader, std::string &error);
  void Reload(const ShaderSource &source);
  void WaitIdle();
  // Cumulative wall time in explicit waits, acquire, and present calls.
  double CpuWaitMilliseconds() const;
  std::vector<Profiling::GpuSample> TakeGpuTimings();
  std::uint32_t ValidationErrors() const;

private:
  struct Impl;
  std::unique_ptr<Impl> impl;
};

} // namespace Vulkan
