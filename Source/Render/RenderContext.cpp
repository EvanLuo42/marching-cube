#include "RenderContext.h"

#include <GLFW/glfw3.h>

#include "backends/imgui_impl_vulkan.h"

auto appName = "Marching Cube Terrain";
auto engineName = "Vulkan Engine";

RenderContext::RenderContext(GLFWwindow *window) {
    instance = vk::raii::su::makeInstance(context, appName, engineName, {}, vk::su::getInstanceExtensions());
#if !defined(NDEBUG)
    debugUtilsMessenger = {instance, vk::su::makeDebugUtilsMessengerCreateInfoEXT()};
#endif
    physicalDevice = vk::raii::PhysicalDevices(instance).front();
    auto [width, height] = std::make_tuple(0, 0);
    glfwGetWindowSize(window, &width, &height);
    auto extent = vk::Extent2D{static_cast<unsigned int>(width), static_cast<unsigned int>(height)};
    surfaceData = {instance, extent, window};
    auto [graphics, present] = vk::raii::su::findGraphicsAndPresentQueueFamilyIndex(physicalDevice, surfaceData->surface);
    graphicsQueueFamilyIndex = graphics;
    presentQueueFamilyIndex = present;
    device = vk::raii::su::makeDevice(physicalDevice, graphics, vk::su::getDeviceExtensions());
    commandPool = vk::raii::CommandPool(device, {{}, graphics});

    graphicsQueue = {device, graphics, 0};
    presentQueue = {device, present, 0};

    // swapChainData = {physicalDevice,
    //                  device,
    //                  surfaceData->surface,
    //                  surfaceData->extent,
    //                  vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
    //                  {},
    //                  graphics,
    //                  present};
    pipelineCache = {device, vk::PipelineCacheCreateInfo{}};
    descriptorPool = vk::raii::su::makeDescriptorPool(device, {{vk::DescriptorType::eCombinedImageSampler, IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE}});

}
