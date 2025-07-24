#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "GLFW/glfw3.h"
#include "Utils.h"

class RenderContext {
public:
    vk::raii::Context context;
    vk::raii::Instance instance = nullptr;
    vk::raii::DebugUtilsMessengerEXT debugUtilsMessenger = nullptr;
    vk::raii::PhysicalDevice physicalDevice = nullptr;
    std::optional<vk::raii::su::SurfaceData> surfaceData;
    vk::raii::Device device = nullptr;
    std::optional<vk::raii::su::SwapChainData> swapChainData;
    vk::raii::CommandPool commandPool = nullptr;

    uint32_t graphicsQueueFamilyIndex;
    uint32_t presentQueueFamilyIndex;

    vk::raii::Queue graphicsQueue = nullptr;
    vk::raii::Queue presentQueue = nullptr;

    vk::raii::PipelineCache pipelineCache = nullptr;

    explicit RenderContext(GLFWwindow *window);
};
