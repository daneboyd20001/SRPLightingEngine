#pragma once

#include <chrono>
#include <cstdint>
#include <deque>
#include <fstream>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace Profiling {

using Clock = std::chrono::steady_clock;
double Milliseconds(Clock::duration duration);

// Main-thread wall time spent inside explicit blocking renderer calls.
// This is not process CPU usage: driver overhead inside those calls is
// included.
class WaitScope {
public:
  explicit WaitScope(double &total) : total(total), start(Clock::now()) {}
  ~WaitScope() { total += Milliseconds(Clock::now() - start); }
  WaitScope(const WaitScope &) = delete;
  WaitScope &operator=(const WaitScope &) = delete;

private:
  double &total;
  Clock::time_point start;
};

struct GpuTimes {
  double computeMs = 0;
  double transferMs = 0;
  // Command-buffer interval, including barriers and intervening GPU waits.
  // Excludes compositor/scanout; compute and transfer need not sum to total.
  double totalMs = 0;
};

struct GpuSample {
  std::uint64_t frameNumber = 0;
  std::optional<GpuTimes> times; // Unavailable is distinct from zero.
};

struct FrameSample {
  std::uint64_t frameNumber = 0;
  double elapsedSeconds = 0;
  double frameMs = 0;
  double cpuWorkMs = 0;
  double cpuWaitMs = 0;
  std::optional<GpuTimes> gpu;
};

struct Averages {
  std::uint64_t frames = 0;
  std::uint64_t gpuFrames = 0;
  double fps = 0;
  double frameMs = 0;
  double cpuWorkMs = 0;
  double cpuWaitMs = 0;
  std::optional<GpuTimes> gpu;
};

class Profiler {
public:
  // Empty CSV path disables file logging. Summary is printed by Finish().
  explicit Profiler(const std::string &csvPath = "", bool printSummary = true);
  void BeginFrame(double rendererWaitMs);
  void CancelFrame(); // Ignore minimized/out-of-date loops and their idle time.
  std::uint64_t FrameNumber() const { return current.frameNumber; }
  void EndFrame(double rendererWaitMs, std::span<const GpuSample> completed);
  void Finish(std::span<const GpuSample> completed);
  // Last second of completed samples, refreshed on demand at most four times
  // per second.
  const Averages &Rolling() {
    UpdateRolling();
    return rolling;
  }

private:
  void Collect(std::span<const GpuSample> completed);
  void Record(const FrameSample &sample);
  void UpdateRolling(bool force = false);
  void PrintSummary() const;

  bool printSummary;
  std::ofstream csv;
  Clock::time_point started = Clock::now();
  Clock::time_point frameStart{}, lastEnd{}, nextRefresh{};
  double waitStart = 0, lastWait = 0;
  std::uint64_t nextFrame = 1;
  FrameSample current;
  // Only CPU frames still waiting for their matching GPU result live here.
  struct PendingFrame {
    FrameSample sample;
    bool gpuComplete = false;
  };
  std::map<std::uint64_t, PendingFrame> pending;
  std::deque<FrameSample> recent;
  Averages rolling;
  std::uint64_t frameCount = 0, gpuCount = 0;
  double frameTotal = 0, workTotal = 0, waitTotal = 0;
  GpuTimes gpuTotal;
  // One value per frame is retained for exact exit percentiles, not full logs.
  std::vector<double> frameTimes;
};

} // namespace Profiling
