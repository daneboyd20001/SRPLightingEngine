#pragma once

#include "../profiling.hpp"

#include <vulkan/vulkan.h>

namespace Vulkan {

// One query pool per frame slot. Reset/reuse only after that slot's fence.
class GpuTimer {
public:
  enum Point {
    TotalStart,
    ComputeStart,
    ComputeEnd,
    TransferStart,
    TransferEnd,
    TotalEnd,
    QueryCount
  };
  GpuTimer(VkDevice device, unsigned validBits, float timestampPeriod);
  ~GpuTimer();
  GpuTimer(const GpuTimer &) = delete;
  GpuTimer &operator=(const GpuTimer &) = delete;
  void Reset(VkCommandBuffer command);
  void Write(VkCommandBuffer command, Point point);
  void Submitted(std::uint64_t frameNumber) { pending = frameNumber; }
  // Call after the submission fence or an already-required device-idle wait.
  std::optional<Profiling::GpuSample> Read();

private:
  VkDevice device;
  VkQueryPool pool = VK_NULL_HANDLE;
  std::uint64_t mask;
  double millisecondsPerTick;
  std::optional<std::uint64_t> pending;
};

} // namespace Vulkan
