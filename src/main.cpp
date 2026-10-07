#include "camera.hpp"
#include "profiling.hpp"
#include "settings.hpp"
#include "uniforms.hpp"
#include "vulkan/vulkan.hpp"
#include "window.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>

int main() try {
  Settings settings;
  Window window(settings.width, settings.height, "SDF Engine");
  Vulkan::ShaderSource source{
      .entry = SDF_SHADER_DIR "/raymarcher.glsl",
      .preIncludes = {"noise.glsl"},
      .compiler = SDF_GLSLC,
  };
  Vulkan::ShaderWatcher watcher(source);
  auto shader = Vulkan::CompileShader(source);
  if (!shader.binary)
    throw std::runtime_error(shader.diagnostics);
  if (shader.binary->localSize != std::array<std::uint32_t, 3>{8, 8, 1})
    throw std::runtime_error("The SDF shader requires an 8x8x1 local size");

  Vulkan::RendererConfig config{
      .uniforms = {{.binding = 1, .bytes = sizeof(CameraUniforms)},
                   {.binding = 2, .bytes = sizeof(LightingUniforms)},
                   {.binding = 3, .bytes = sizeof(SdfUniforms)}},
      .flipOutputY = true, // Shader coordinates run bottom-up.
      .vsync = settings.vsync,
      .validation = settings.validation,
  };
  Vulkan::Renderer renderer(window.handle, config, *shader.binary);
  Profiling::Profiler profiler(settings.profileCsv, settings.profileSummary);
  Camera camera;
  double lastTime = glfwGetTime();
  bool reloadDown = false;
  std::cout << "Tab: capture/release | WASD: move | Space/Ctrl: up/down\n"
               "Shift: sprint | R: reload shaders | Escape: quit\n";

  while (!glfwWindowShouldClose(window.handle)) {
    profiler.BeginFrame(renderer.CpuWaitMilliseconds());
    glfwPollEvents();
    double now = glfwGetTime();
    float deltaTime = float(std::clamp(now - lastTime, 0.0, 0.1));
    lastTime = now;
    camera.Controls(window.handle, deltaTime);
    if (glfwWindowShouldClose(window.handle))
      break;

    bool reload = glfwGetKey(window.handle, GLFW_KEY_R) == GLFW_PRESS;
    if (watcher.Changed(reload && !reloadDown)) {
      renderer.Reload(source);
      lastTime = glfwGetTime(); // Compilation time must not move the camera.
    }
    reloadDown = reload;

    if (!renderer.BeginFrame(profiler.FrameNumber())) {
      profiler.CancelFrame();
      glfwWaitEventsTimeout(0.05);
      continue;
    }
    auto extent = renderer.Extent();
    CameraUniforms view(camera, extent.width, extent.height);
    LightingUniforms light{
        .lampStrength = settings.lampStrength,
        .activeLighting = settings.lighting,
    };
    SdfUniforms sdf{
        .time = float(now),
        .quality = settings.quality,
        .minClip = settings.minClip,
        .maxClip = settings.maxClip,
        .activeSDF = settings.sdf,
    };
    std::array uploads{Vulkan::Upload(1, view), Vulkan::Upload(2, light),
                       Vulkan::Upload(3, sdf)};
    renderer.Submit(uploads,
                    {(extent.width + 7) / 8, (extent.height + 7) / 8, 1});
    profiler.EndFrame(renderer.CpuWaitMilliseconds(),
                      renderer.TakeGpuTimings());
  }
  renderer.WaitIdle();
  profiler.Finish(renderer.TakeGpuTimings());
  return renderer.ValidationErrors() ? 1 : 0;
} catch (const std::exception &error) {
  std::cerr << "SDF Engine: " << error.what() << '\n';
  return 1;
}
