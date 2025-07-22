#include "RenderContext.h"

#include <iostream>

RenderContext::RenderContext(GLFWwindow *window) {
    context.emplace();
    vk::ApplicationInfo applicationInfo{};
    applicationInfo.sType = vk::StructureType::eApplicationInfo;
    applicationInfo.pApplicationName = glfwGetWindowTitle(window);
    applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    applicationInfo.pEngineName = "Vulkan Engine";
    applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    applicationInfo.apiVersion = VK_API_VERSION_1_3;

    const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};

    vk::InstanceCreateInfo instanceCreateInfo{};
    instanceCreateInfo.sType = vk::StructureType::eInstanceCreateInfo;
    instanceCreateInfo.pApplicationInfo = &applicationInfo;
    instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    instanceCreateInfo.ppEnabledLayerNames = validationLayers.data();

    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector instanceExtensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

#if defined(__APPLE__)
    instanceExtensions.emplace_back(vk::KHRPortabilityEnumerationExtensionName);
#endif

    instanceExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    instanceCreateInfo.flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
    instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
    instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();

    try {
        instance.emplace(*context, instanceCreateInfo);
    } catch (vk::SystemError &error) {
        throw std::runtime_error(std::format("failed to create instance: {}", error.what()));
    }

    vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo(
            {}, vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                    vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
            [](VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT,
               const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *) -> VkBool32 {
                std::cerr << "[Vulkan Validation] " << pCallbackData->pMessage << std::endl;
                return VK_FALSE;
            });

    debugMessenger = vk::raii::DebugUtilsMessengerEXT(*instance, debugCreateInfo);

    VkSurfaceKHR rawSurface;
    if (glfwCreateWindowSurface(**instance, window, nullptr, &rawSurface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }

    surface.emplace(*instance, rawSurface);

    std::vector deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME, "VK_KHR_portability_subset", "VK_KHR_shader_draw_parameters"};

    for (auto &device: instance->enumeratePhysicalDevices()) {
        auto props = device.getQueueFamilyProperties();
        auto supportedExtensions = device.enumerateDeviceExtensionProperties();

        const bool allSupported = std::ranges::all_of(deviceExtensions, [&](const char *required) {
            return std::ranges::any_of(supportedExtensions,
                                       [&](const auto &ext) { return std::strcmp(required, ext.extensionName) == 0; });
        });

        if (!allSupported)
            continue;

        for (uint32_t i = 0; i < props.size(); ++i) {
            if (props[i].queueFlags & vk::QueueFlagBits::eGraphics && device.getSurfaceSupportKHR(i, *surface)) {
                physicalDevice.emplace(std::move(device));
                graphicsQueueFamily = i;
                goto found;
            }
        }
    }
    exit(-1);
found:
    auto priority = 1.0f;
    vk::DeviceQueueCreateInfo queueInfo{{}, graphicsQueueFamily, 1, &priority};
    vk::DeviceCreateInfo deviceInfo{
            {}, 1, &queueInfo, 0, nullptr, static_cast<uint32_t>(deviceExtensions.size()), deviceExtensions.data()};
    try {
        device.emplace(*physicalDevice, deviceInfo);
    } catch (const vk::SystemError &error) {
        throw std::runtime_error(std::format("failed to create device: {}", error.what()));
    }
    try {
        graphicsQueue.emplace(device->getQueue(graphicsQueueFamily, 0));
    } catch (const vk::SystemError &error) {
        throw std::runtime_error(std::format("failed to get graphics queue: {}", error.what()));
    }

    vk::SwapchainCreateInfoKHR swapchainCreateInfo{};
    swapchainCreateInfo.surface = *surface;
    swapchainCreateInfo.minImageCount = 2;
    swapchainCreateInfo.imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
    swapchainCreateInfo.imageFormat = vk::Format::eB8G8R8A8Srgb;
    swapchainCreateInfo.imageExtent = vk::Extent2D{800, 600};
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
    swapchainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
    swapchainCreateInfo.preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
    swapchainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    swapchainCreateInfo.presentMode = vk::PresentModeKHR::eFifo;
    swapchainCreateInfo.clipped = VK_TRUE;
    swapchainCreateInfo.oldSwapchain = nullptr;

    swapchain = vk::raii::SwapchainKHR{*device, swapchainCreateInfo};

    swapchainImages = swapchain->getImages();

    std::vector<vk::ImageView> imageViews;
    imageViews.reserve(swapchainImages.size());
    vk::ImageViewCreateInfo imageViewCreateInfo{};
    imageViewCreateInfo.sType = vk::StructureType::eImageViewCreateInfo;
    imageViewCreateInfo.viewType = vk::ImageViewType::e2D;
    imageViewCreateInfo.format = vk::Format::eB8G8R8A8Srgb;
    imageViewCreateInfo.components.r = vk::ComponentSwizzle::eIdentity;
    imageViewCreateInfo.components.g = vk::ComponentSwizzle::eIdentity;
    imageViewCreateInfo.components.b = vk::ComponentSwizzle::eIdentity;
    imageViewCreateInfo.components.a = vk::ComponentSwizzle::eIdentity;
    imageViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    imageViewCreateInfo.subresourceRange.levelCount = 1;
    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCreateInfo.subresourceRange.layerCount = 1;

    for (auto image: swapchainImages) {
        imageViewCreateInfo.image = image;
        imageViews.push_back(device->createImageView(imageViewCreateInfo));
    }

    swapchainImageViews = imageViews;
}
