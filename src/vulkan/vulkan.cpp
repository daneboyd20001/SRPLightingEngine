#include "vulkan.hpp"
#include "timing.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstring>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>

namespace Vulkan {
namespace {

void Check(VkResult result, const char *operation) {
  if (result != VK_SUCCESS)
    throw std::runtime_error(std::string(operation) + " failed (VkResult " +
                             std::to_string(result) + ")");
}

constexpr unsigned FrameCount = 2;

VkImageMemoryBarrier2 ImageBarrier(VkImage image, VkImageLayout from,
                                   VkImageLayout to,
                                   VkPipelineStageFlags2 sourceStage,
                                   VkAccessFlags2 sourceAccess,
                                   VkPipelineStageFlags2 destinationStage,
                                   VkAccessFlags2 destinationAccess) {
  return {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
      .srcStageMask = sourceStage,
      .srcAccessMask = sourceAccess,
      .dstStageMask = destinationStage,
      .dstAccessMask = destinationAccess,
      .oldLayout = from,
      .newLayout = to,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = image,
      .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                           .levelCount = 1,
                           .layerCount = 1},
  };
}

void Barrier(VkCommandBuffer command,
             std::span<const VkImageMemoryBarrier2> images) {
  VkDependencyInfo dependency{
      .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
      .imageMemoryBarrierCount = static_cast<unsigned>(images.size()),
      .pImageMemoryBarriers = images.data(),
  };
  vkCmdPipelineBarrier2(command, &dependency);
}

} // namespace

struct Renderer::Impl {
  GLFWwindow *window = nullptr;
  RendererConfig config;
  VkInstance instance = VK_NULL_HANDLE;
  VkDebugUtilsMessengerEXT messenger = VK_NULL_HANDLE;
  VkSurfaceKHR surface = VK_NULL_HANDLE;
  VkPhysicalDevice physical = VK_NULL_HANDLE;
  VkPhysicalDeviceProperties properties{};
  VkDevice device = VK_NULL_HANDLE;
  VkQueue queue = VK_NULL_HANDLE;
  unsigned queueFamily = 0;
  unsigned timestampBits = 0;
  double cpuWaitMs = 0;
  std::vector<Profiling::GpuSample> completedTimings;
  VkSwapchainKHR swapchain = VK_NULL_HANDLE;
  VkExtent2D extent{};
  VkExtent2D lastFramebuffer{};
  std::vector<VkImage> swapImages;
  // Present waits can outlive the submission fence. These semaphores belong
  // to swapchain images and are reused only after that image is reacquired.
  std::vector<VkSemaphore> presentReady;
  VkDescriptorSetLayout descriptorLayout = VK_NULL_HANDLE;
  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
  VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
  std::string shaderInterface;
  VkCommandPool commandPool = VK_NULL_HANDLE;
  struct Buffer {
    VkDevice device;
    VkBuffer handle = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    void *mapped = nullptr;
    bool coherent = false;
    explicit Buffer(VkDevice device) : device(device) {}
    Buffer(const Buffer &) = delete;
    Buffer &operator=(const Buffer &) = delete;
    ~Buffer() {
      if (mapped)
        vkUnmapMemory(device, memory);
      vkDestroyBuffer(device, handle, nullptr);
      vkFreeMemory(device, memory, nullptr);
    }
  };
  struct Image {
    VkDevice device;
    VkImage handle = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    explicit Image(VkDevice device) : device(device) {}
    Image(const Image &) = delete;
    Image &operator=(const Image &) = delete;
    ~Image() {
      vkDestroyImageView(device, view, nullptr);
      vkDestroyImage(device, handle, nullptr);
      vkFreeMemory(device, memory, nullptr);
    }
  };
  struct Pipeline {
    VkDevice device;
    VkPipeline handle = VK_NULL_HANDLE;
    explicit Pipeline(VkDevice device) : device(device) {}
    Pipeline(const Pipeline &) = delete;
    Pipeline &operator=(const Pipeline &) = delete;
    ~Pipeline() { vkDestroyPipeline(device, handle, nullptr); }
  };
  struct ShaderModule {
    VkDevice device;
    VkShaderModule handle = VK_NULL_HANDLE;
    explicit ShaderModule(VkDevice device) : device(device) {}
    ShaderModule(const ShaderModule &) = delete;
    ShaderModule &operator=(const ShaderModule &) = delete;
    ~ShaderModule() { vkDestroyShaderModule(device, handle, nullptr); }
  };
  std::unique_ptr<Pipeline> pipeline;
  struct Frame {
    VkCommandBuffer command = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    VkSemaphore acquired = VK_NULL_HANDLE;
    VkDescriptorSet descriptors = VK_NULL_HANDLE;
    std::vector<std::unique_ptr<Buffer>> uniforms;
    std::unique_ptr<Image> output;
    std::unique_ptr<GpuTimer> timer;
    std::uint64_t frameNumber = 0;
  };
  std::array<Frame, FrameCount> frames;
  unsigned frameIndex = 0, imageIndex = 0;
  bool frameOpen = false, rebuild = false;
  std::atomic<unsigned> validationErrors{0};

  static VKAPI_ATTR VkBool32 VKAPI_CALL
  Debug(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT,
        const VkDebugUtilsMessengerCallbackDataEXT *data, void *user) {
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
      ++static_cast<Impl *>(user)->validationErrors;
    std::cerr << "[Vulkan] " << data->pMessage << '\n';
    return VK_FALSE;
  }

  ~Impl() {
    if (device) {
      vkDeviceWaitIdle(device);
      DestroySwapchain();
      pipeline.reset();
      if (pipelineLayout)
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
      if (descriptorPool)
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
      if (descriptorLayout)
        vkDestroyDescriptorSetLayout(device, descriptorLayout, nullptr);
      for (auto &frame : frames) {
        // Release GPU-owned resources while their device is still alive.
        frame.uniforms.clear();
        frame.timer.reset();
        if (frame.fence)
          vkDestroyFence(device, frame.fence, nullptr);
        if (frame.acquired)
          vkDestroySemaphore(device, frame.acquired, nullptr);
      }
      if (commandPool)
        vkDestroyCommandPool(device, commandPool, nullptr);
      vkDestroyDevice(device, nullptr);
    }
    if (surface)
      vkDestroySurfaceKHR(instance, surface, nullptr);
    if (messenger) {
      auto destroy = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
          vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
      if (destroy)
        destroy(instance, messenger, nullptr);
    }
    if (instance)
      vkDestroyInstance(instance, nullptr);
  }

  void CreateInstance() {
    if (!glfwVulkanSupported())
      throw std::runtime_error("GLFW cannot find a Vulkan loader/driver");
    unsigned count = 0;
    const char **required = glfwGetRequiredInstanceExtensions(&count);
    if (!required)
      throw std::runtime_error("GLFW cannot provide Vulkan surface extensions");
    std::vector<const char *> extensions(required, required + count);
    const char *validationLayer = "VK_LAYER_KHRONOS_validation";
    if (config.validation) {
      Check(vkEnumerateInstanceLayerProperties(&count, nullptr),
            "Enumerate instance layers");
      std::vector<VkLayerProperties> layers(count);
      Check(vkEnumerateInstanceLayerProperties(&count, layers.data()),
            "Enumerate instance layers");
      if (std::none_of(layers.begin(), layers.end(), [&](auto &layer) {
            return std::strcmp(layer.layerName, validationLayer) == 0;
          }))
        throw std::runtime_error("Validation layer unavailable; install it or "
                                 "disable validation in settings.hpp");
      extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
      extensions.push_back(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);
    }
    VkApplicationInfo application{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "SDF Engine",
        .apiVersion = VK_API_VERSION_1_3,
    };
    VkDebugUtilsMessengerCreateInfoEXT debug{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = Debug,
        .pUserData = this,
    };
    const VkValidationFeatureEnableEXT sync =
        VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT;
    VkValidationFeaturesEXT features{
        .sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
        .pNext = &debug,
        .enabledValidationFeatureCount = 1,
        .pEnabledValidationFeatures = &sync,
    };
    VkInstanceCreateInfo create{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = config.validation ? &features : nullptr,
        .pApplicationInfo = &application,
        .enabledLayerCount = config.validation ? 1u : 0u,
        .ppEnabledLayerNames = config.validation ? &validationLayer : nullptr,
        .enabledExtensionCount = static_cast<unsigned>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
    };
    Check(vkCreateInstance(&create, nullptr, &instance),
          "Create Vulkan 1.3 instance");
    if (config.validation) {
      auto make = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
          vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
      if (!make)
        throw std::runtime_error("Debug utils extension unavailable");
      Check(make(instance, &debug, nullptr, &messenger),
            "Create debug messenger");
    }
    Check(glfwCreateWindowSurface(instance, window, nullptr, &surface),
          "Create GLFW surface");
  }

  bool SurfaceCompatible(VkPhysicalDevice candidate) {
    VkSurfaceCapabilitiesKHR capabilities;
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
            candidate, surface, &capabilities) != VK_SUCCESS ||
        !(capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT))
      return false;
    VkFormatProperties format;
    vkGetPhysicalDeviceFormatProperties(candidate, config.outputFormat,
                                        &format);
    constexpr auto required =
        VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT | VK_FORMAT_FEATURE_BLIT_SRC_BIT;
    if ((format.optimalTilingFeatures & required) != required)
      return false;
    unsigned count = 0;
    Check(vkGetPhysicalDeviceSurfaceFormatsKHR(candidate, surface, &count,
                                               nullptr),
          "Get surface formats");
    std::vector<VkSurfaceFormatKHR> formats(count);
    Check(vkGetPhysicalDeviceSurfaceFormatsKHR(candidate, surface, &count,
                                               formats.data()),
          "Get surface formats");
    for (const auto &entry : formats) {
      const auto f = entry.format == VK_FORMAT_UNDEFINED
                         ? VK_FORMAT_B8G8R8A8_UNORM
                         : entry.format;
      if (f != VK_FORMAT_B8G8R8A8_UNORM && f != VK_FORMAT_R8G8B8A8_UNORM &&
          f != VK_FORMAT_B8G8R8A8_SRGB && f != VK_FORMAT_R8G8B8A8_SRGB)
        continue;
      if (entry.colorSpace != VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        continue;
      vkGetPhysicalDeviceFormatProperties(candidate, f, &format);
      if (format.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT)
        return true;
    }
    return false;
  }

  void CreateDevice() {
    unsigned count;
    Check(vkEnumeratePhysicalDevices(instance, &count, nullptr),
          "Enumerate GPUs");
    std::vector<VkPhysicalDevice> devices(count);
    Check(vkEnumeratePhysicalDevices(instance, &count, devices.data()),
          "Enumerate GPUs");
    int bestScore = -1;
    for (auto candidate : devices) {
      VkPhysicalDeviceProperties props;
      vkGetPhysicalDeviceProperties(candidate, &props);
      if (props.apiVersion < VK_API_VERSION_1_3)
        continue;
      VkPhysicalDeviceVulkan13Features features13{
          .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
      VkPhysicalDeviceFeatures2 features{
          .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
          .pNext = &features13,
      };
      vkGetPhysicalDeviceFeatures2(candidate, &features);
      if (!features13.synchronization2 || !features13.maintenance4)
        continue;
      Check(vkEnumerateDeviceExtensionProperties(candidate, nullptr, &count,
                                                 nullptr),
            "Enumerate device extensions");
      std::vector<VkExtensionProperties> extensions(count);
      Check(vkEnumerateDeviceExtensionProperties(candidate, nullptr, &count,
                                                 extensions.data()),
            "Enumerate device extensions");
      if (std::none_of(
              extensions.begin(), extensions.end(), [](auto &extension) {
                return std::strcmp(extension.extensionName,
                                   VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0;
              }))
        continue;
      if (!SurfaceCompatible(candidate))
        continue;
      vkGetPhysicalDeviceQueueFamilyProperties(candidate, &count, nullptr);
      std::vector<VkQueueFamilyProperties> families(count);
      vkGetPhysicalDeviceQueueFamilyProperties(candidate, &count,
                                               families.data());
      for (unsigned family = 0; family < count; ++family) {
        VkBool32 present = false;
        Check(vkGetPhysicalDeviceSurfaceSupportKHR(candidate, family, surface,
                                                   &present),
              "Check present support");
        constexpr auto required = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;
        if (!present || (families[family].queueFlags & required) != required)
          continue;
        int score = 0;
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
          score = 1;
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
          score = 2;
        if (score > bestScore) {
          bestScore = score;
          physical = candidate;
          properties = props;
          queueFamily = family;
          timestampBits = families[family].timestampValidBits;
        }
      }
    }
    if (!physical)
      throw std::runtime_error(
          "No suitable Vulkan 1.3 GPU: need synchronization2, maintenance4, "
          "a graphics/compute/present queue, and storage-image-to-swapchain "
          "blit support");
    // Stage-specific timestamps are optional even on Vulkan 1.3 devices.
    if (!properties.limits.timestampComputeAndGraphics ||
        properties.limits.timestampPeriod <= 0)
      timestampBits = 0;
    if (!timestampBits)
      std::cout << "GPU profiling unavailable on this queue.\n";
    const float priority = 1.0f;
    VkDeviceQueueCreateInfo queueInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queueFamily,
        .queueCount = 1,
        .pQueuePriorities = &priority,
    };
    VkPhysicalDeviceVulkan13Features features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .synchronization2 = true,
        .maintenance4 = true,
    };
    const char *extension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    VkDeviceCreateInfo create{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &features,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueInfo,
        .enabledExtensionCount = 1,
        .ppEnabledExtensionNames = &extension,
    };
    Check(vkCreateDevice(physical, &create, nullptr, &device), "Create device");
    vkGetDeviceQueue(device, queueFamily, 0, &queue);
    std::cout << "Vulkan 1.3: " << properties.deviceName << "\n";
  }

  unsigned MemoryType(unsigned bits, VkMemoryPropertyFlags required,
                      VkMemoryPropertyFlags preferred = 0) {
    VkPhysicalDeviceMemoryProperties memory;
    vkGetPhysicalDeviceMemoryProperties(physical, &memory);
    unsigned fallback = std::numeric_limits<unsigned>::max();
    for (unsigned i = 0; i < memory.memoryTypeCount; ++i) {
      const auto flags = memory.memoryTypes[i].propertyFlags;
      if ((bits & (1u << i)) && (flags & required) == required) {
        if ((flags & preferred) == preferred)
          return i;
        fallback = i;
      }
    }
    if (fallback == std::numeric_limits<unsigned>::max())
      throw std::runtime_error("No suitable Vulkan memory type");
    return fallback;
  }

  void CreateBuffer(Buffer &buffer, VkDeviceSize bytes) {
    VkBufferCreateInfo create{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = bytes,
        .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    Check(vkCreateBuffer(device, &create, nullptr, &buffer.handle),
          "Create uniform buffer");
    VkMemoryRequirements requirements;
    vkGetBufferMemoryRequirements(device, buffer.handle, &requirements);
    VkMemoryAllocateInfo allocation{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = requirements.size,
        .memoryTypeIndex = MemoryType(requirements.memoryTypeBits,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
    };
    VkPhysicalDeviceMemoryProperties memory;
    vkGetPhysicalDeviceMemoryProperties(physical, &memory);
    buffer.coherent =
        memory.memoryTypes[allocation.memoryTypeIndex].propertyFlags &
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    Check(vkAllocateMemory(device, &allocation, nullptr, &buffer.memory),
          "Allocate uniform memory");
    Check(vkBindBufferMemory(device, buffer.handle, buffer.memory, 0),
          "Bind uniform memory");
    Check(
        vkMapMemory(device, buffer.memory, 0, VK_WHOLE_SIZE, 0, &buffer.mapped),
        "Map uniform memory");
  }

  void CreateImage(Image &image) {
    VkImageFormatProperties support;
    Check(vkGetPhysicalDeviceImageFormatProperties(
              physical, config.outputFormat, VK_IMAGE_TYPE_2D,
              VK_IMAGE_TILING_OPTIMAL,
              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, 0,
              &support),
          "Check compute image support");
    if (extent.width > support.maxExtent.width ||
        extent.height > support.maxExtent.height)
      throw std::runtime_error("Compute image extent exceeds GPU limits");
    VkImageCreateInfo create{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = config.outputFormat,
        .extent = {.width = extent.width, .height = extent.height, .depth = 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    Check(vkCreateImage(device, &create, nullptr, &image.handle),
          "Create compute image");
    VkMemoryRequirements requirements;
    vkGetImageMemoryRequirements(device, image.handle, &requirements);
    VkMemoryAllocateInfo allocation{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = requirements.size,
        .memoryTypeIndex = MemoryType(requirements.memoryTypeBits,
                                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    };
    Check(vkAllocateMemory(device, &allocation, nullptr, &image.memory),
          "Allocate compute image");
    Check(vkBindImageMemory(device, image.handle, image.memory, 0),
          "Bind compute image");
    VkImageViewCreateInfo view{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image.handle,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = config.outputFormat,
        .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                             .levelCount = 1,
                             .layerCount = 1},
    };
    Check(vkCreateImageView(device, &view, nullptr, &image.view),
          "Create compute image view");
  }

  void DestroySwapchain() {
    for (auto &frame : frames)
      frame.output.reset();
    for (auto semaphore : presentReady)
      if (semaphore)
        vkDestroySemaphore(device, semaphore, nullptr);
    presentReady.clear();
    swapImages.clear();
    if (swapchain)
      vkDestroySwapchainKHR(device, swapchain, nullptr);
    swapchain = VK_NULL_HANDLE;
  }

  bool CreateSwapchain() {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    if (!width || !height || glfwGetWindowAttrib(window, GLFW_ICONIFIED))
      return false;
    WaitIdle();
    VkSurfaceCapabilitiesKHR capabilities;
    Check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical, surface,
                                                    &capabilities),
          "Get surface capabilities");
    if (!(capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT))
      throw std::runtime_error(
          "Surface no longer supports transfer-destination presentation");
    extent = capabilities.currentExtent;
    if (extent.width == std::numeric_limits<unsigned>::max()) {
      extent.width = std::clamp(static_cast<unsigned>(width),
                                capabilities.minImageExtent.width,
                                capabilities.maxImageExtent.width);
      extent.height = std::clamp(static_cast<unsigned>(height),
                                 capabilities.minImageExtent.height,
                                 capabilities.maxImageExtent.height);
    }
    if (!extent.width || !extent.height)
      return false;
    lastFramebuffer = {.width = static_cast<unsigned>(width),
                       .height = static_cast<unsigned>(height)};
    unsigned count;
    Check(vkGetPhysicalDeviceSurfaceFormatsKHR(physical, surface, &count,
                                               nullptr),
          "Get surface formats");
    std::vector<VkSurfaceFormatKHR> formats(count);
    Check(vkGetPhysicalDeviceSurfaceFormatsKHR(physical, surface, &count,
                                               formats.data()),
          "Get surface formats");
    VkSurfaceFormatKHR selected{};
    bool found = false;
    // UNORM preserves the shader's numeric output without an extra sRGB encode.
    // If only sRGB is available, Vulkan's blit performs the destination
    // encoding.
    for (auto preferred : {VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM,
                           VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_R8G8B8A8_SRGB}) {
      VkFormatProperties formatProperties;
      vkGetPhysicalDeviceFormatProperties(physical, preferred,
                                          &formatProperties);
      if (!(formatProperties.optimalTilingFeatures &
            VK_FORMAT_FEATURE_BLIT_DST_BIT))
        continue;
      for (auto format : formats) {
        if ((format.format == preferred ||
             format.format == VK_FORMAT_UNDEFINED) &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
          selected = {.format = preferred, .colorSpace = format.colorSpace};
          found = true;
          break;
        }
      }
      if (found)
        break;
    }
    if (!found)
      throw std::runtime_error(
          "No supported swapchain blit destination format");
    Check(vkGetPhysicalDeviceSurfacePresentModesKHR(physical, surface, &count,
                                                    nullptr),
          "Get present modes");
    std::vector<VkPresentModeKHR> modes(count);
    Check(vkGetPhysicalDeviceSurfacePresentModesKHR(physical, surface, &count,
                                                    modes.data()),
          "Get present modes");
    VkPresentModeKHR mode = VK_PRESENT_MODE_FIFO_KHR;
    if (!config.vsync) {
      for (auto preferred :
           {VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_MAILBOX_KHR}) {
        if (std::find(modes.begin(), modes.end(), preferred) != modes.end()) {
          mode = preferred;
          break;
        }
      }
    }
    unsigned imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount)
      imageCount = std::min(imageCount, capabilities.maxImageCount);
    VkCompositeAlphaFlagBitsKHR alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    for (auto option : {VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
                        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
                        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR}) {
      if (capabilities.supportedCompositeAlpha & option) {
        alpha = option;
        break;
      }
    }
    VkSwapchainCreateInfoKHR create{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = imageCount,
        .imageFormat = selected.format,
        .imageColorSpace = selected.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = alpha,
        .presentMode = mode,
        .clipped = true,
        .oldSwapchain = swapchain,
    };
    VkSwapchainKHR replacement = VK_NULL_HANDLE;
    Check(vkCreateSwapchainKHR(device, &create, nullptr, &replacement),
          "Create swapchain");
    DestroySwapchain();
    swapchain = replacement;
    Check(vkGetSwapchainImagesKHR(device, swapchain, &count, nullptr),
          "Get swapchain images");
    swapImages.resize(count);
    Check(vkGetSwapchainImagesKHR(device, swapchain, &count, swapImages.data()),
          "Get swapchain images");
    presentReady.resize(count, VK_NULL_HANDLE);
    VkSemaphoreCreateInfo semaphore{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    for (auto &ready : presentReady)
      Check(vkCreateSemaphore(device, &semaphore, nullptr, &ready),
            "Create present semaphore");
    for (auto &frame : frames) {
      frame.output = std::make_unique<Image>(device);
      CreateImage(*frame.output);
      VkDescriptorImageInfo image{
          .imageView = frame.output->view,
          .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
      };
      VkWriteDescriptorSet write{
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = frame.descriptors,
          .dstBinding = config.outputBinding,
          .descriptorCount = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
          .pImageInfo = &image,
      };
      vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    }
    rebuild = false;
    return true;
  }

  void ValidateInterface(const ShaderBinary &shader) {
    std::set<unsigned> bindings{config.outputBinding};
    for (const auto &buffer : config.uniforms) {
      if (!buffer.bytes ||
          buffer.bytes > properties.limits.maxUniformBufferRange ||
          !bindings.insert(buffer.binding).second)
        throw std::runtime_error("Invalid or duplicate uniform buffer binding");
    }
    if (config.uniforms.size() >
        properties.limits.maxPerStageDescriptorUniformBuffers)
      throw std::runtime_error("Too many uniform buffers for this GPU");
    bool outputFound = false;
    for (const auto &binding : shader.bindings) {
      if (binding.set != 0)
        throw std::runtime_error("Only descriptor set 0 is configured");
      if (binding.kind == ShaderBinding::Kind::StorageImage) {
        // Explicit format mapping avoids accepting a shader/image mismatch.
        const std::pair<VkFormat, unsigned> supported[] = {
            {VK_FORMAT_R32G32B32A32_SFLOAT, 1},
            {VK_FORMAT_R16G16B16A16_SFLOAT, 2},
            {VK_FORMAT_R32_SFLOAT, 3},
            {VK_FORMAT_R8G8B8A8_UNORM, 4}};
        bool matches = false;
        for (auto [format, spirv] : supported)
          matches |=
              config.outputFormat == format && binding.imageFormat == spirv;
        if (binding.binding != config.outputBinding || !matches || outputFound)
          throw std::runtime_error(
              "Shader storage image does not match renderer configuration");
        outputFound = true;
      } else {
        auto buffer =
            std::find_if(config.uniforms.begin(), config.uniforms.end(),
                         [&](auto &b) { return b.binding == binding.binding; });
        if (buffer == config.uniforms.end() ||
            buffer->bytes < binding.minimumBytes)
          throw std::runtime_error(
              "Shader uniform buffer exceeds/misses configured binding " +
              std::to_string(binding.binding));
      }
    }
    if (!outputFound)
      throw std::runtime_error("Shader has no configured output image");
    std::uint64_t invocations = 1;
    for (unsigned i = 0; i < 3; ++i) {
      if (!shader.localSize[i] ||
          shader.localSize[i] > properties.limits.maxComputeWorkGroupSize[i])
        throw std::runtime_error("Shader local size exceeds GPU limits");
      invocations *= shader.localSize[i];
    }
    if (invocations > properties.limits.maxComputeWorkGroupInvocations)
      throw std::runtime_error("Shader workgroup exceeds GPU invocation limit");
  }

  void CreateFrames() {
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    bindings.push_back({.binding = config.outputBinding,
                        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                        .descriptorCount = 1,
                        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT});
    for (const auto &buffer : config.uniforms)
      bindings.push_back({.binding = buffer.binding,
                          .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                          .descriptorCount = 1,
                          .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT});
    VkDescriptorSetLayoutCreateInfo layout{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<unsigned>(bindings.size()),
        .pBindings = bindings.data(),
    };
    Check(vkCreateDescriptorSetLayout(device, &layout, nullptr,
                                      &descriptorLayout),
          "Create descriptor layout");
    VkPipelineLayoutCreateInfo pipelineInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &descriptorLayout,
    };
    Check(
        vkCreatePipelineLayout(device, &pipelineInfo, nullptr, &pipelineLayout),
        "Create pipeline layout");
    std::vector<VkDescriptorPoolSize> sizes{
        {.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
         .descriptorCount = FrameCount}};
    if (!config.uniforms.empty())
      sizes.push_back(
          {.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
           .descriptorCount =
               FrameCount * static_cast<unsigned>(config.uniforms.size())});
    VkDescriptorPoolCreateInfo pool{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = FrameCount,
        .poolSizeCount = static_cast<unsigned>(sizes.size()),
        .pPoolSizes = sizes.data(),
    };
    Check(vkCreateDescriptorPool(device, &pool, nullptr, &descriptorPool),
          "Create descriptor pool");
    VkCommandPoolCreateInfo commands{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queueFamily,
    };
    Check(vkCreateCommandPool(device, &commands, nullptr, &commandPool),
          "Create command pool");
    for (auto &frame : frames) {
      frame.timer = std::make_unique<GpuTimer>(device, timestampBits,
                                              properties.limits.timestampPeriod);
      VkDescriptorSetAllocateInfo allocation{
          .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
          .descriptorPool = descriptorPool,
          .descriptorSetCount = 1,
          .pSetLayouts = &descriptorLayout,
      };
      Check(vkAllocateDescriptorSets(device, &allocation, &frame.descriptors),
            "Allocate descriptors");
      VkCommandBufferAllocateInfo command{
          .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
          .commandPool = commandPool,
          .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
          .commandBufferCount = 1,
      };
      Check(vkAllocateCommandBuffers(device, &command, &frame.command),
            "Allocate command buffer");
      VkFenceCreateInfo fence{
          .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
          .flags = VK_FENCE_CREATE_SIGNALED_BIT,
      };
      Check(vkCreateFence(device, &fence, nullptr, &frame.fence),
            "Create frame fence");
      VkSemaphoreCreateInfo semaphore{
          .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
      Check(vkCreateSemaphore(device, &semaphore, nullptr, &frame.acquired),
            "Create acquire semaphore");
      frame.uniforms.resize(config.uniforms.size());
      for (unsigned i = 0; i < config.uniforms.size(); ++i) {
        frame.uniforms[i] = std::make_unique<Buffer>(device);
        CreateBuffer(*frame.uniforms[i], config.uniforms[i].bytes);
        VkDescriptorBufferInfo buffer{
            .buffer = frame.uniforms[i]->handle,
            .range = config.uniforms[i].bytes,
        };
        VkWriteDescriptorSet write{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = frame.descriptors,
            .dstBinding = config.uniforms[i].binding,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &buffer,
        };
        vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
      }
    }
  }

  void CollectTiming(Frame &frame) {
    if (frame.timer)
      if (auto sample = frame.timer->Read())
        completedTimings.push_back(*sample);
  }

  void WaitIdle() {
    {
      Profiling::WaitScope wait(cpuWaitMs);
      Check(vkDeviceWaitIdle(device), "Wait for device");
    }
    for (auto &frame : frames)
      CollectTiming(frame);
  }

  std::unique_ptr<Pipeline> CreatePipeline(const ShaderBinary &shader) {
    VkShaderModuleCreateInfo moduleInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = shader.words.size() * sizeof(std::uint32_t),
        .pCode = shader.words.data(),
    };
    ShaderModule module(device);
    Check(vkCreateShaderModule(device, &moduleInfo, nullptr, &module.handle),
          "Create compute shader module");
    VkComputePipelineCreateInfo create{
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                  .stage = VK_SHADER_STAGE_COMPUTE_BIT,
                  .module = module.handle,
                  .pName = "main"},
        .layout = pipelineLayout,
    };
    auto result = std::make_unique<Pipeline>(device);
    Check(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &create, nullptr,
                                   &result->handle),
          "Create compute pipeline");
    return result;
  }
};

Renderer::Renderer(GLFWwindow *window, const RendererConfig &config,
                   const ShaderBinary &shader)
    : impl(std::make_unique<Impl>()) {
  impl->window = window;
  impl->config = config;
  impl->CreateInstance();
  impl->CreateDevice();
  impl->ValidateInterface(shader);
  impl->CreateFrames();
  impl->pipeline = impl->CreatePipeline(shader);
  impl->shaderInterface = shader.interface;
  impl->CreateSwapchain();
}

Renderer::~Renderer() = default;

bool Renderer::BeginFrame(std::uint64_t frameNumber) {
  auto &r = *impl;
  if (r.frameOpen)
    throw std::logic_error("Previous frame has not been submitted");
  int width, height;
  glfwGetFramebufferSize(r.window, &width, &height);
  if (!width || !height || glfwGetWindowAttrib(r.window, GLFW_ICONIFIED))
    return false;
  if (!r.swapchain || r.rebuild ||
      static_cast<unsigned>(width) != r.lastFramebuffer.width ||
      static_cast<unsigned>(height) != r.lastFramebuffer.height) {
    if (!r.CreateSwapchain())
      return false;
  }
  auto &frame = r.frames[r.frameIndex];
  {
    Profiling::WaitScope wait(r.cpuWaitMs);
    Check(vkWaitForFences(r.device, 1, &frame.fence, true, UINT64_MAX),
          "Wait for frame");
  }
  r.CollectTiming(frame);
  VkResult result;
  {
    Profiling::WaitScope wait(r.cpuWaitMs);
    result = vkAcquireNextImageKHR(r.device, r.swapchain, UINT64_MAX, frame.acquired,
                                   VK_NULL_HANDLE, &r.imageIndex);
  }
  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    r.rebuild = true;
    return false;
  }
  if (result == VK_SUBOPTIMAL_KHR)
    r.rebuild = true;
  else
    Check(result, "Acquire swapchain image");
  r.frameOpen = true;
  frame.frameNumber = frameNumber;
  return true;
}

VkExtent2D Renderer::Extent() const { return impl->extent; }

void Renderer::Submit(std::span<const BufferUpload> uploads,
                      std::array<std::uint32_t, 3> groups) {
  auto &r = *impl;
  if (!r.frameOpen)
    throw std::logic_error("BeginFrame must succeed before Submit");
  auto &frame = r.frames[r.frameIndex];
  for (unsigned i = 0; i < 3; ++i)
    if (!groups[i] ||
        groups[i] > r.properties.limits.maxComputeWorkGroupCount[i])
      throw std::runtime_error("Dispatch dimensions exceed GPU limits");
  if (uploads.size() != r.config.uniforms.size())
    throw std::runtime_error(
        "Supply each configured uniform buffer exactly once");
  for (unsigned i = 0; i < r.config.uniforms.size(); ++i) {
    const auto &spec = r.config.uniforms[i];
    auto upload = std::find_if(uploads.begin(), uploads.end(), [&](auto &u) {
      return u.binding == spec.binding;
    });
    if (upload == uploads.end() || upload->bytes.size() != spec.bytes)
      throw std::runtime_error(
          "Uniform upload does not match configured binding " +
          std::to_string(spec.binding));
    auto &buffer = *frame.uniforms[i];
    std::memcpy(buffer.mapped, upload->bytes.data(), spec.bytes);
    if (!buffer.coherent) {
      VkMappedMemoryRange range{
          .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
          .memory = buffer.memory,
          .size = VK_WHOLE_SIZE,
      };
      Check(vkFlushMappedMemoryRanges(r.device, 1, &range),
            "Flush uniform upload");
    }
  }
  Check(vkResetCommandBuffer(frame.command, 0), "Reset command buffer");
  VkCommandBufferBeginInfo begin{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  };
  Check(vkBeginCommandBuffer(frame.command, &begin), "Begin commands");
  frame.timer->Reset(frame.command);
  frame.timer->Write(frame.command, GpuTimer::TotalStart);
  const auto prepare = ImageBarrier(
      frame.output->handle, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
      VK_PIPELINE_STAGE_2_NONE, 0, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
      VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT);
  Barrier(frame.command, {&prepare, 1});
  vkCmdBindPipeline(frame.command, VK_PIPELINE_BIND_POINT_COMPUTE,
                    r.pipeline->handle);
  vkCmdBindDescriptorSets(frame.command, VK_PIPELINE_BIND_POINT_COMPUTE,
                          r.pipelineLayout, 0, 1, &frame.descriptors, 0,
                          nullptr);
  frame.timer->Write(frame.command, GpuTimer::ComputeStart);
  vkCmdDispatch(frame.command, groups[0], groups[1], groups[2]);
  frame.timer->Write(frame.command, GpuTimer::ComputeEnd);
  const std::array transfer{
      ImageBarrier(frame.output->handle, VK_IMAGE_LAYOUT_GENERAL,
                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                   VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
                   VK_PIPELINE_STAGE_2_BLIT_BIT, VK_ACCESS_2_TRANSFER_READ_BIT),
      ImageBarrier(
          r.swapImages[r.imageIndex], VK_IMAGE_LAYOUT_UNDEFINED,
          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
          // Match the acquire wait so this transition follows presentation.
          VK_PIPELINE_STAGE_2_BLIT_BIT, 0, VK_PIPELINE_STAGE_2_BLIT_BIT,
          VK_ACCESS_2_TRANSFER_WRITE_BIT)};
  Barrier(frame.command, transfer);
  const int width = static_cast<int>(r.extent.width);
  const int height = static_cast<int>(r.extent.height);
  VkImageBlit blit{
      .srcSubresource = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                         .layerCount = 1},
      .srcOffsets = {{.y = r.config.flipOutputY ? height : 0},
                     {.x = width,
                      .y = r.config.flipOutputY ? 0 : height,
                      .z = 1}},
      .dstSubresource = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                         .layerCount = 1},
      .dstOffsets = {{}, {.x = width, .y = height, .z = 1}},
  };
  frame.timer->Write(frame.command, GpuTimer::TransferStart);
  vkCmdBlitImage(
      frame.command, frame.output->handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      r.swapImages[r.imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
      &blit, VK_FILTER_NEAREST);
  frame.timer->Write(frame.command, GpuTimer::TransferEnd);
  const auto present = ImageBarrier(
      r.swapImages[r.imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_2_BLIT_BIT,
      VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_2_NONE, 0);
  Barrier(frame.command, {&present, 1});
  frame.timer->Write(frame.command, GpuTimer::TotalEnd);
  Check(vkEndCommandBuffer(frame.command), "End commands");
  VkSemaphoreSubmitInfo wait{
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
      .semaphore = frame.acquired,
      .stageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
  };
  VkSemaphoreSubmitInfo signal{
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
      .semaphore = r.presentReady[r.imageIndex],
      .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
  };
  VkCommandBufferSubmitInfo command{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
      .commandBuffer = frame.command,
  };
  VkSubmitInfo2 submit{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
      .waitSemaphoreInfoCount = 1,
      .pWaitSemaphoreInfos = &wait,
      .commandBufferInfoCount = 1,
      .pCommandBufferInfos = &command,
      .signalSemaphoreInfoCount = 1,
      .pSignalSemaphoreInfos = &signal,
  };
  Check(vkResetFences(r.device, 1, &frame.fence), "Reset frame fence");
  Check(vkQueueSubmit2(r.queue, 1, &submit, frame.fence),
        "Submit compute frame");
  frame.timer->Submitted(frame.frameNumber);
  VkPresentInfoKHR info{
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &signal.semaphore,
      .swapchainCount = 1,
      .pSwapchains = &r.swapchain,
      .pImageIndices = &r.imageIndex,
  };
  VkResult result;
  {
    Profiling::WaitScope wait(r.cpuWaitMs);
    result = vkQueuePresentKHR(r.queue, &info);
  }
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    r.rebuild = true;
  else
    Check(result, "Present compute image");
  r.frameOpen = false;
  r.frameIndex = (r.frameIndex + 1) % FrameCount;
}

bool Renderer::ReplaceShader(const ShaderBinary &shader, std::string &error) {
  auto &r = *impl;
  try {
    if (r.frameOpen)
      throw std::logic_error("Replace shaders between frames");
    if (shader.interface != r.shaderInterface)
      throw std::runtime_error(
          "Shader interface changed (bindings, buffer layout, or local size); "
          "restart with matching CPU configuration");
    r.ValidateInterface(shader);
    auto replacement = r.CreatePipeline(shader);
    // Submitted frames must finish before their pipeline is destroyed.
    r.WaitIdle();
    r.pipeline = std::move(replacement);
    return true;
  } catch (const std::exception &failure) {
    error = failure.what();
    return false;
  }
}

void Renderer::Reload(const ShaderSource &source) {
  auto result = CompileShader(source);
  if (!result.diagnostics.empty())
    std::cerr << result.diagnostics;
  if (!result.binary) {
    std::cerr << "Keeping previous shader.\n";
    return;
  }
  std::string error;
  if (ReplaceShader(*result.binary, error))
    std::cout << "Shader reloaded.\n";
  else
    std::cerr << "Reload rejected; keeping previous shader: " << error << '\n';
}

void Renderer::WaitIdle() {
  impl->WaitIdle();
}

double Renderer::CpuWaitMilliseconds() const { return impl->cpuWaitMs; }

std::vector<Profiling::GpuSample> Renderer::TakeGpuTimings() {
  std::vector<Profiling::GpuSample> result;
  result.swap(impl->completedTimings);
  return result;
}
std::uint32_t Renderer::ValidationErrors() const {
  return impl->validationErrors.load();
}

} // namespace Vulkan
