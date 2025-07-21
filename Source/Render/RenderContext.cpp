#include "RenderContext.h"

RenderContext::RenderContext(GLFWwindow* window, const std::string &appName) {
    context.emplace();
    vk::ApplicationInfo applicationInfo{};
    applicationInfo.sType = vk::StructureType::eApplicationInfo;
    applicationInfo.pApplicationName = appName.c_str();
    applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    applicationInfo.pEngineName = "Vulkan Engine";
    applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    applicationInfo.apiVersion = VK_API_VERSION_1_3;

    vk::InstanceCreateInfo instanceCreateInfo{};
    instanceCreateInfo.sType = vk::StructureType::eInstanceCreateInfo;
    instanceCreateInfo.pApplicationInfo = &applicationInfo;

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector instanceExtensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

#if defined(__APPLE__)
    instanceExtensions.emplace_back(vk::KHRPortabilityEnumerationExtensionName);
#endif

    instanceCreateInfo.flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
    instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
    instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();

    try {
        instance.emplace(*context, instanceCreateInfo);
    } catch (vk::SystemError & error) {
        throw std::runtime_error("failed to create instance!");
    }

    VkSurfaceKHR rawSurface;
    if (glfwCreateWindowSurface(**instance, window, nullptr, &rawSurface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }

    surface.emplace(*instance, rawSurface);

    std::vector deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    for (auto &device : instance->enumeratePhysicalDevices())
    {
        auto props = device.getQueueFamilyProperties();
        auto supportedExtensions = device.enumerateDeviceExtensionProperties();

        const bool allSupported = std::ranges::all_of(deviceExtensions, [&](const char *required) {
            return std::ranges::any_of(supportedExtensions,
                                       [&](const auto &ext) { return std::strcmp(required, ext.extensionName) == 0; });
        });

        if (!allSupported)
            continue;

        for (uint32_t i = 0; i < props.size(); ++i)
        {
            if (props[i].queueFlags & vk::QueueFlagBits::eGraphics && device.getSurfaceSupportKHR(i, *surface))
            {
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
    try
    {
        device.emplace(*physicalDevice, deviceInfo);
    }
    catch (const vk::SystemError &error)
    {
        throw std::runtime_error(error);
    }
    try
    {
        graphicsQueue.emplace(device->getQueue(graphicsQueueFamily, 0));
    }
    catch (const vk::SystemError &error)
    {
        throw std::runtime_error("failed to create graphics queue!");
    }
}
