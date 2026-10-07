#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace Vulkan {

struct ShaderSource {
  std::filesystem::path entry;
  // Extra dependencies emitted before the entry body, after its #version.
  std::vector<std::filesystem::path> preIncludes;
  std::filesystem::path compiler;
};

struct ShaderBinding {
  enum class Kind { UniformBuffer, StorageImage };
  std::uint32_t set = 0, binding = 0;
  Kind kind{};
  std::uint32_t minimumBytes = 0;
  std::uint32_t imageFormat = 0; // SPIR-V Image Format enum
};

struct ShaderBinary {
  std::vector<std::uint32_t> words;
  std::vector<ShaderBinding> bindings;
  std::array<std::uint32_t, 3> localSize{};
  // Types, field names/offsets, and local size. Compared before reload.
  std::string interface;
};

struct ShaderResult {
  std::optional<ShaderBinary> binary;
  std::string diagnostics;
};

ShaderResult CompileShader(const ShaderSource &source);

// Checks for saved edits every 250 ms, then waits for saves to settle.
class ShaderWatcher {
public:
  explicit ShaderWatcher(ShaderSource source);
  bool Changed(bool force = false);

private:
  using Stamp = std::pair<std::filesystem::file_time_type, std::uintmax_t>;
  using Snapshot = std::map<std::filesystem::path, Stamp>;
  void ScanFile(const std::filesystem::path &path, Snapshot &snapshot) const;
  Snapshot Scan() const;
  ShaderSource source;
  Snapshot observed;
  std::chrono::steady_clock::time_point nextPoll{}, changedAt{};
  bool dirty = false;
};

} // namespace Vulkan
