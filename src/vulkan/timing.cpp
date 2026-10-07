#include "timing.hpp"

#include <array>
#include <stdexcept>

namespace Vulkan {

GpuTimer::GpuTimer(VkDevice device, unsigned validBits, float timestampPeriod)
    : device(device),
      mask(validBits >= 64 ? UINT64_MAX : (std::uint64_t{1} << validBits) - 1),
      millisecondsPerTick(timestampPeriod / 1'000'000.0) {
  if (!validBits)
    return;
  VkQueryPoolCreateInfo info{
      .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
      .queryType = VK_QUERY_TYPE_TIMESTAMP,
      .queryCount = QueryCount,
  };
  if (vkCreateQueryPool(device, &info, nullptr, &pool) != VK_SUCCESS)
    throw std::runtime_error("Cannot create GPU timestamp query pool");
}

GpuTimer::~GpuTimer() { vkDestroyQueryPool(device, pool, nullptr); }

void GpuTimer::Reset(VkCommandBuffer command) {
  if (pool)
    vkCmdResetQueryPool(command, pool, 0, QueryCount);
}

void GpuTimer::Write(VkCommandBuffer command, Point point) {
  if (!pool)
    return;
  static constexpr VkPipelineStageFlags2 stages[QueryCount] = {
      VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
      VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
      VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
      VK_PIPELINE_STAGE_2_BLIT_BIT,
      VK_PIPELINE_STAGE_2_BLIT_BIT,
      VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
  };
  vkCmdWriteTimestamp2(command, stages[point], pool, point);
}

std::optional<Profiling::GpuSample> GpuTimer::Read() {
  if (!pending)
    return std::nullopt;
  Profiling::GpuSample sample{.frameNumber = *pending};
  pending.reset();
  if (!pool)
    return sample;
  struct Query {
    std::uint64_t ticks, available;
  };
  std::array<Query, QueryCount> queries{};
  const auto status = vkGetQueryPoolResults(
      device, pool, 0, QueryCount, sizeof(queries), queries.data(), sizeof(Query),
      VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WITH_AVAILABILITY_BIT);
  if (status == VK_NOT_READY)
    return sample;
  if (status != VK_SUCCESS)
    throw std::runtime_error("Cannot read GPU timestamp queries");
  for (const auto &query : queries)
    if (!query.available)
      return sample;
  auto duration = [&](Point start, Point end) {
    // Unsigned subtraction and masking handle the queue's timestamp wraparound.
    return ((queries[end].ticks - queries[start].ticks) & mask) *
           millisecondsPerTick;
  };
  sample.times = {.computeMs = duration(ComputeStart, ComputeEnd),
                  .transferMs = duration(TransferStart, TransferEnd),
                  .totalMs = duration(TotalStart, TotalEnd)};
  return sample;
}

} // namespace Vulkan
