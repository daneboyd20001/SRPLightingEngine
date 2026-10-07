#include "profiling.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <locale>
#include <sstream>
#include <stdexcept>

namespace Profiling {

double Milliseconds(Clock::duration duration) {
  return std::chrono::duration<double, std::milli>(duration).count();
}

Profiler::Profiler(const std::string &csvPath, bool printSummary)
    : printSummary(printSummary) {
  if (csvPath.empty())
    return;
  csv.open(csvPath);
  if (!csv)
    throw std::runtime_error("Cannot open profiling CSV: " + csvPath);
  csv.imbue(std::locale::classic());
  csv << std::fixed << std::setprecision(6)
      << "frame,elapsed_s,frame_ms,fps,cpu_work_ms,cpu_wait_ms,"
         "gpu_compute_ms,gpu_transfer_ms,gpu_total_ms\n";
}

void Profiler::BeginFrame(double rendererWaitMs) {
  const auto now = Clock::now();
  // Include the preceding logging/bookkeeping in the next frame's wall time.
  frameStart = lastEnd == Clock::time_point{} ? now : lastEnd;
  waitStart = lastEnd == Clock::time_point{} ? rendererWaitMs : lastWait;
  current = {.frameNumber = nextFrame++};
}

void Profiler::CancelFrame() { lastEnd = {}; }

void Profiler::EndFrame(double rendererWaitMs,
                        std::span<const GpuSample> completed) {
  lastEnd = Clock::now();
  lastWait = rendererWaitMs;
  current.elapsedSeconds = Milliseconds(lastEnd - started) / 1000.0;
  current.frameMs = Milliseconds(lastEnd - frameStart);
  current.cpuWaitMs =
      std::clamp(rendererWaitMs - waitStart, 0.0, current.frameMs);
  current.cpuWorkMs = current.frameMs - current.cpuWaitMs;
  pending.emplace(current.frameNumber, PendingFrame{.sample = current});
  Collect(completed);
}

void Profiler::Collect(std::span<const GpuSample> completed) {
  // A resize/shutdown may return multiple slots out of order. Keep CSV ordered.
  for (const auto &gpu : completed) {
    auto frame = pending.find(gpu.frameNumber);
    if (frame == pending.end())
      throw std::logic_error("GPU timing has no matching CPU frame");
    frame->second.sample.gpu = gpu.times;
    frame->second.gpuComplete = true;
  }
  while (!pending.empty() && pending.begin()->second.gpuComplete) {
    auto frame = pending.begin();
    Record(frame->second.sample);
    pending.erase(frame);
  }
}

void Profiler::Record(const FrameSample &sample) {
  ++frameCount;
  frameTotal += sample.frameMs;
  workTotal += sample.cpuWorkMs;
  waitTotal += sample.cpuWaitMs;
  if (sample.gpu) {
    ++gpuCount;
    gpuTotal.computeMs += sample.gpu->computeMs;
    gpuTotal.transferMs += sample.gpu->transferMs;
    gpuTotal.totalMs += sample.gpu->totalMs;
  }
  if (printSummary)
    frameTimes.push_back(sample.frameMs);
  recent.push_back(sample);
  while (!recent.empty() &&
         recent.front().elapsedSeconds < sample.elapsedSeconds - 1.0)
    recent.pop_front();

  if (csv.is_open()) {
    const double fps = sample.frameMs > 0 ? 1000.0 / sample.frameMs : 0;
    csv << sample.frameNumber << ',' << sample.elapsedSeconds << ','
        << sample.frameMs << ',' << fps << ',' << sample.cpuWorkMs << ','
        << sample.cpuWaitMs << ',';
    if (sample.gpu)
      csv << sample.gpu->computeMs << ',' << sample.gpu->transferMs << ','
          << sample.gpu->totalMs;
    else
      csv << ",,"; // Empty numeric fields mean unavailable.
    csv << '\n';
    if (!csv)
      throw std::runtime_error("Cannot write profiling CSV");
  }
}

void Profiler::UpdateRolling(bool force) {
  const auto now = Clock::now();
  if (!force && now < nextRefresh)
    return;
  nextRefresh = now + std::chrono::milliseconds(250);
  const double elapsed = Milliseconds(now - started) / 1000.0;
  while (!recent.empty() && recent.front().elapsedSeconds < elapsed - 1.0)
    recent.pop_front();
  rolling = {};
  GpuTimes gpu;
  for (const auto &sample : recent) {
    ++rolling.frames;
    rolling.frameMs += sample.frameMs;
    rolling.cpuWorkMs += sample.cpuWorkMs;
    rolling.cpuWaitMs += sample.cpuWaitMs;
    if (sample.gpu) {
      ++rolling.gpuFrames;
      gpu.computeMs += sample.gpu->computeMs;
      gpu.transferMs += sample.gpu->transferMs;
      gpu.totalMs += sample.gpu->totalMs;
    }
  }
  if (rolling.frames) {
    rolling.fps =
        rolling.frameMs > 0 ? rolling.frames * 1000.0 / rolling.frameMs : 0;
    rolling.frameMs /= rolling.frames;
    rolling.cpuWorkMs /= rolling.frames;
    rolling.cpuWaitMs /= rolling.frames;
  }
  if (rolling.gpuFrames)
    rolling.gpu = {.computeMs = gpu.computeMs / rolling.gpuFrames,
                   .transferMs = gpu.transferMs / rolling.gpuFrames,
                   .totalMs = gpu.totalMs / rolling.gpuFrames};
}

void Profiler::PrintSummary() const {
  std::ostringstream out;
  out << std::fixed << std::setprecision(3) << "\nProfile: " << frameCount
      << " rendered frames (startup/idle loops excluded)\n";
  if (frameCount) {
    auto sorted = frameTimes;
    std::sort(sorted.begin(), sorted.end());
    auto percentile = [&](double fraction) {
      // Nearest-rank percentile, including stalls from reload/resize.
      auto rank = static_cast<std::size_t>(std::ceil(fraction * sorted.size()));
      return sorted[rank - 1];
    };
    out << "  Frame: " << frameTotal / frameCount << " ms average, "
        << (frameTotal > 0 ? frameCount * 1000.0 / frameTotal : 0) << " FPS\n"
        << "  Frame p50/p95/p99: " << percentile(0.50) << " / "
        << percentile(0.95) << " / " << percentile(0.99) << " ms\n"
        << "  CPU wall work/wait: " << workTotal / frameCount << " / "
        << waitTotal / frameCount << " ms average\n";
  }
  if (gpuCount)
    out << "  GPU compute/transfer/total: " << gpuTotal.computeMs / gpuCount
        << " / " << gpuTotal.transferMs / gpuCount << " / "
        << gpuTotal.totalMs / gpuCount << " ms average (" << gpuCount
        << " samples)\n";
  else
    out << "  GPU timings: unavailable\n";
  std::cout << out.str();
}

void Profiler::Finish(std::span<const GpuSample> completed) {
  Collect(completed);
  // Incomplete GPU measurements are kept as unavailable, never fabricated
  // zeros.
  for (const auto &[id, frame] : pending)
    Record(frame.sample);
  pending.clear();
  UpdateRolling(true);
  if (csv.is_open()) {
    csv.flush();
    if (!csv)
      throw std::runtime_error("Cannot flush profiling CSV");
  }
  if (printSummary)
    PrintSummary();
}

} // namespace Profiling
