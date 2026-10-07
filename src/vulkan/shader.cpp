#include "shader.hpp"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <functional>
#include <set>
#include <spawn.h>
#include <sstream>
#include <stdexcept>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;

namespace Vulkan {
namespace {

std::string ReadText(const std::filesystem::path &path) {
  std::ifstream file(path);
  if (!file)
    throw std::runtime_error("Cannot read shader: " + path.string());
  return {std::istreambuf_iterator<char>(file), {}};
}

struct Assembly {
  std::set<std::filesystem::path> emitted, active;
  std::ostringstream text;

  void Emit(const std::filesystem::path &name) {
    const auto path = std::filesystem::canonical(name);
    if (active.contains(path))
      throw std::runtime_error("Cyclic shader include: " + path.string());
    if (!emitted.insert(path).second)
      return;
    active.insert(path);
    std::istringstream input(ReadText(path));
    std::string line;
    unsigned number = 0;
    // Quoted #line paths are supported by glslc and preserve useful
    // diagnostics.
    auto location = [&](unsigned n) {
      text << "#line " << n << " \"" << path.string() << "\"\n";
    };
    location(1);
    while (std::getline(input, line)) {
      ++number;
      const auto start = line.find_first_not_of(" \t\r");
      const auto directive =
          start == std::string::npos ? "" : line.substr(start);
      if (directive.starts_with("#version")) {
        text << '\n';
      } else if (directive.starts_with("#include")) {
        const auto first = directive.find('"');
        const auto last =
            directive.find('"', first == std::string::npos ? 0 : first + 1);
        if (first == std::string::npos || last == std::string::npos)
          throw std::runtime_error("Expected a quoted include in " +
                                   path.string());
        Emit(path.parent_path() /
             directive.substr(first + 1, last - first - 1));
        location(number + 1);
      } else {
        text << line << '\n';
      }
    }
    active.erase(path);
  }
};

struct TempDirectory {
  std::filesystem::path path;
  TempDirectory() {
    auto pattern =
        (std::filesystem::temp_directory_path() / "sdf-shader-XXXXXX").string();
    std::vector<char> writable(pattern.begin(), pattern.end());
    writable.push_back('\0');
    const char *created = mkdtemp(writable.data());
    if (!created)
      throw std::runtime_error("Cannot create shader compilation directory");
    path = created;
  }
  ~TempDirectory() {
    std::error_code error;
    std::filesystem::remove_all(path, error);
  }
};

int RunCompiler(std::vector<std::string> arguments,
                const std::filesystem::path &log) {
  std::vector<char *> argv;
  for (auto &argument : arguments)
    argv.push_back(argument.data());
  argv.push_back(nullptr);
  posix_spawn_file_actions_t actions;
  int error = posix_spawn_file_actions_init(&actions);
  if (error)
    throw std::runtime_error(std::strerror(error));
  error = posix_spawn_file_actions_addopen(&actions, STDOUT_FILENO, log.c_str(),
                                           O_WRONLY | O_CREAT | O_TRUNC, 0600);
  if (!error)
    error = posix_spawn_file_actions_adddup2(&actions, STDOUT_FILENO,
                                             STDERR_FILENO);
  pid_t pid{};
  if (!error)
    error =
        posix_spawnp(&pid, argv[0], &actions, nullptr, argv.data(), environ);
  posix_spawn_file_actions_destroy(&actions);
  if (error)
    throw std::runtime_error("Cannot launch glslc: " +
                             std::string(std::strerror(error)));
  int status{};
  while (waitpid(pid, &status, 0) == -1) {
    if (errno != EINTR)
      throw std::runtime_error("Cannot wait for glslc");
  }
  return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

// Reflect only the explicitly supported compute interface. The structural
// signature catches ABI changes without depending on compiler-assigned IDs.
void Reflect(ShaderBinary &binary) {
  struct Instruction {
    unsigned opcode;
    std::vector<std::uint32_t> args;
  };
  std::map<unsigned, Instruction> types;
  std::map<unsigned, std::uint32_t> constants;
  std::map<unsigned, std::map<unsigned, std::string>> memberNames;
  std::map<unsigned, std::map<unsigned, std::vector<std::uint32_t>>>
      decorations;
  std::map<unsigned,
           std::map<unsigned, std::map<unsigned, std::vector<std::uint32_t>>>>
      members;
  struct Variable {
    unsigned type, id, storage;
  };
  std::vector<Variable> variables;
  std::array<unsigned, 3> localIds{};
  bool computeEntry = false;
  const auto &words = binary.words;
  if (words.size() < 5 || words[0] != 0x07230203)
    throw std::runtime_error("Invalid SPIR-V header");
  for (std::size_t i = 5; i < words.size();) {
    const unsigned count = words[i] >> 16, op = words[i] & 0xffff;
    if (!count || i + count > words.size())
      throw std::runtime_error("Invalid SPIR-V instruction");
    const auto *p = words.data() + i;
    if (op == 6 && count >= 4) { // OpMemberName
      const char *name = reinterpret_cast<const char *>(p + 3);
      memberNames[p[1]][p[2]] = std::string(name, strnlen(name, (count - 3) * 4));
    }
    if (op >= 19 && op <= 33 && count >= 2)
      types[p[1]] = {op, {p + 2, p + count}};
    if (op == 43 && count == 4)
      constants[p[2]] = p[3];
    if (op == 71 && count >= 3)
      decorations[p[1]][p[2]] = {p + 3, p + count};
    if (op == 72 && count >= 4)
      members[p[1]][p[2]][p[3]] = {p + 4, p + count};
    if (op == 59 && count >= 4)
      variables.push_back({p[1], p[2], p[3]});
    if (op == 15 && count >= 4 && p[1] == 5)
      computeEntry = true;
    if (op == 16 && count == 6 && p[2] == 17)
      binary.localSize = {p[3], p[4], p[5]};
    if (op == 331 && count == 6 && p[2] == 38)
      localIds = {p[3], p[4], p[5]};
    i += count;
  }
  if (localIds[0])
    for (unsigned i = 0; i < 3; ++i)
      binary.localSize[i] = constants.at(localIds[i]);
  if (!computeEntry || !binary.localSize[0] || !binary.localSize[1] ||
      !binary.localSize[2])
    throw std::runtime_error(
        "Expected a compute shader with a fixed local size");

  auto offset = [&](unsigned type, unsigned member) {
    return members.at(type).at(member).at(35).at(0);
  };
  std::function<unsigned(unsigned)> size = [&](unsigned id) -> unsigned {
    const auto &t = types.at(id);
    const auto &a = t.args;
    switch (t.opcode) {
    case 21:
    case 22:
      return a.at(0) / 8;
    case 23:
      return size(a.at(0)) * a.at(1);
    case 28:
      return decorations.at(id).at(6).at(0) * constants.at(a.at(1));
    case 30: {
      unsigned bytes = 0;
      for (unsigned m = 0; m < a.size(); ++m) {
        const auto &memberType = types.at(a[m]);
        unsigned memberBytes;
        if (memberType.opcode == 24) {
          const auto &d = members.at(id).at(m);
          unsigned vectors = memberType.args.at(1);
          if (d.contains(4))
            vectors = types.at(memberType.args.at(0)).args.at(1);
          memberBytes = d.at(7).at(0) * vectors;
        } else {
          memberBytes = size(a[m]);
        }
        bytes = std::max(bytes, offset(id, m) + memberBytes);
      }
      return bytes;
    }
    default:
      throw std::runtime_error("Unsupported uniform buffer member type");
    }
  };

  auto describeDecorations = [](const auto &map) {
    std::ostringstream out;
    for (const auto &[kind, values] : map) {
      // Only memory layout decorations contribute to the interface.
      if (kind != 2 && kind != 3 && kind != 4 && kind != 5 && kind != 6 &&
          kind != 7 && kind != 35)
        continue;
      out << 'd' << kind << ':';
      for (auto value : values)
        out << value << ',';
    }
    return out.str();
  };
  std::function<std::string(unsigned)> signature = [&](unsigned id) {
    const auto &t = types.at(id);
    std::ostringstream out;
    out << '(' << t.opcode << ':' << describeDecorations(decorations[id]);
    for (unsigned n = 0; n < t.args.size(); ++n) {
      bool typeId = t.opcode == 30 ||
                    ((t.opcode == 23 || t.opcode == 24 || t.opcode == 25 ||
                      t.opcode == 27 || t.opcode == 28 || t.opcode == 29) &&
                     n == 0);
      if (t.opcode == 30) {
        // Catch reordering of equally sized fields as well as offset changes.
        const auto &name = memberNames[id][n];
        out << name.size() << ':' << name;
        out << describeDecorations(members[id][n]);
      }
      if (typeId)
        out << signature(t.args[n]);
      else if (t.opcode == 28 && n == 1)
        out << constants.at(t.args[n]);
      else
        out << t.args[n];
      out << ',';
    }
    out << ')';
    return out.str();
  };

  std::map<std::pair<unsigned, unsigned>, std::string> resources;
  for (const auto &v : variables) {
    if (v.storage == 9 || v.storage == 12)
      throw std::runtime_error(
          "Push constants and storage buffers are not configured");
    if (v.storage != 0 && v.storage != 2)
      continue;
    const auto &d = decorations.at(v.id);
    ShaderBinding binding;
    binding.binding = d.at(33).at(0);
    binding.set = d.at(34).at(0);
    const unsigned pointee = types.at(v.type).args.at(1);
    const auto &type = types.at(pointee);
    if (v.storage == 2 && type.opcode == 30 &&
        decorations[pointee].contains(2)) {
      binding.kind = ShaderBinding::Kind::UniformBuffer;
      binding.minimumBytes = size(pointee);
    } else if (v.storage == 0 && type.opcode == 25 && type.args.at(1) == 1 &&
               type.args.at(3) == 0 && type.args.at(4) == 0 &&
               type.args.at(5) == 2) {
      binding.kind = ShaderBinding::Kind::StorageImage;
      binding.imageFormat = type.args.at(6);
    } else {
      throw std::runtime_error("Unsupported descriptor type in compute shader");
    }
    auto key = std::make_pair(binding.set, binding.binding);
    if (!resources.emplace(key, signature(pointee)).second)
      throw std::runtime_error("Duplicate descriptor binding");
    binary.bindings.push_back(binding);
  }
  std::ostringstream interface;
  for (auto n : binary.localSize)
    interface << n << '/';
  for (const auto &[key, value] : resources)
    interface << key.first << ':' << key.second << ':' << value << ';';
  binary.interface = interface.str();
}

} // namespace

ShaderResult CompileShader(const ShaderSource &source) {
  ShaderResult result;
  try {
    Assembly assembly;
    std::istringstream entry(ReadText(source.entry));
    std::string line, version;
    while (std::getline(entry, line)) {
      const auto start = line.find_first_not_of(" \t\r");
      if (start != std::string::npos &&
          line.substr(start).starts_with("#version")) {
        version = line;
        break;
      }
    }
    if (version.empty())
      throw std::runtime_error("Shader entry point needs a #version directive");
    assembly.text << version << '\n';
    for (const auto &include : source.preIncludes)
      assembly.Emit(include.is_absolute()
                        ? include
                        : source.entry.parent_path() / include);
    assembly.Emit(source.entry);

    TempDirectory temp;
    const auto input = temp.path / "shader.comp";
    const auto output = temp.path / "shader.spv";
    const auto log = temp.path / "compiler.log";
    {
      std::ofstream file(input);
      file << assembly.text.str();
      if (!file)
        throw std::runtime_error("Cannot write assembled shader");
    }
    const int status = RunCompiler(
        {source.compiler.string(), "-fshader-stage=compute",
         "--target-env=vulkan1.3", "-O", "-g", input.string(), "-o", output.string()},
        log);
    result.diagnostics = ReadText(log);
    if (status != 0) {
      result.diagnostics = "Shader compilation failed:\n" + result.diagnostics;
      return result;
    }
    std::ifstream file(output, std::ios::binary | std::ios::ate);
    const auto bytes = file.tellg();
    if (!file || bytes <= 0 || bytes % 4 != 0)
      throw std::runtime_error("Compiler produced invalid SPIR-V size");
    ShaderBinary binary;
    binary.words.resize(static_cast<std::size_t>(bytes) / 4);
    file.seekg(0);
    file.read(reinterpret_cast<char *>(binary.words.data()), bytes);
    if (!file)
      throw std::runtime_error("Cannot read compiled SPIR-V");
    Reflect(binary);
    result.binary = std::move(binary);
  } catch (const std::exception &error) {
    result.diagnostics += std::string(error.what()) + '\n';
  }
  return result;
}

ShaderWatcher::ShaderWatcher(ShaderSource source) : source(std::move(source)) {
  observed = Scan();
}

void ShaderWatcher::ScanFile(const std::filesystem::path &name, Snapshot &snapshot) const {
  std::error_code error;
  auto path = std::filesystem::weakly_canonical(name, error);
  if (error || snapshot.contains(path))
    return;
  auto time = std::filesystem::last_write_time(path, error);
  if (error)
    return;
  auto bytes = std::filesystem::file_size(path, error);
  if (error)
    return;
  snapshot[path] = {time, bytes};

  // Includes can live outside the shader folder or use a different extension.
  std::ifstream file(path);
  std::string line;
  while (std::getline(file, line)) {
    auto start = line.find_first_not_of(" \t\r");
    if (start == std::string::npos || !line.substr(start).starts_with("#include"))
      continue;
    auto first = line.find('"', start);
    if (first == std::string::npos)
      continue;
    auto last = line.find('"', first + 1);
    if (last != std::string::npos)
      ScanFile(path.parent_path() / line.substr(first + 1, last - first - 1), snapshot);
  }
}

ShaderWatcher::Snapshot ShaderWatcher::Scan() const {
  Snapshot snapshot;
  std::error_code error;
  ScanFile(source.entry, snapshot);
  for (const auto &include : source.preIncludes)
    ScanFile(source.entry.parent_path() / include, snapshot);
  // Watching the source tree also catches newly added, removed, and broken
  // includes. No filesystem work is done on the rendering path between polls.
  for (std::filesystem::recursive_directory_iterator
           it(source.entry.parent_path(), error),
       end;
       !error && it != end; it.increment(error)) {
    if (!it->is_regular_file(error))
      continue;
    const auto extension = it->path().extension();
    if (extension != ".glsl" && extension != ".comp")
      continue;
    ScanFile(it->path(), snapshot);
  }
  return snapshot;
}

bool ShaderWatcher::Changed(bool force) {
  using namespace std::chrono_literals;
  const auto now = std::chrono::steady_clock::now();
  if (force || now >= nextPoll) {
    nextPoll = now + 250ms;
    auto current = Scan();
    if (current != observed) {
      observed = std::move(current);
      changedAt = now;
      dirty = true;
    }
  }
  if (force || (dirty && now - changedAt >= 250ms)) {
    dirty = false;
    return true;
  }
  return false;
}

} // namespace Vulkan
