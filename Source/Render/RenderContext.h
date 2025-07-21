#pragma once
#include <optional>

#include <vulkan/vulkan_raii.hpp>

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

class RenderContext {
    std::optional<vk::raii::Context> context;
    std::optional<vk::raii::Instance> instance;
    std::optional<vk::raii::SurfaceKHR> surface;
    std::optional<vk::raii::PhysicalDevice> physicalDevice;
    std::optional<vk::raii::Device> device;
    std::optional<vk::raii::Queue> graphicsQueue;

    uint32_t graphicsQueueFamily = 0;
public:
    explicit RenderContext(GLFWwindow* window, const std::string &appName);

    [[nodiscard]] const vk::raii::Instance &GetInstance() const
    {
        return *instance;
    }

    [[nodiscard]] vk::raii::Device &getDevice()
    {
        return *device;
    }

    [[nodiscard]] vk::raii::PhysicalDevice &getPhysicalDevice()
    {
        return *physicalDevice;
    }

    [[nodiscard]] vk::raii::SurfaceKHR &getSurface()
    {
        return *surface;
    }

    [[nodiscard]] vk::raii::Queue &getGraphicsQueue()
    {
        return *graphicsQueue;
    }

    [[nodiscard]] uint32_t getGraphicsQueueFamily() const
    {
        return graphicsQueueFamily;
    }
};
