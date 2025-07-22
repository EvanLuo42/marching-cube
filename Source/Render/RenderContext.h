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
    std::optional<vk::raii::SwapchainKHR> swapchain;

    std::vector<vk::Image> swapchainImages;
    std::vector<vk::ImageView> swapchainImageViews;

    uint32_t graphicsQueueFamily = 0;

    std::optional<vk::raii::DebugUtilsMessengerEXT> debugMessenger;

public:
    explicit RenderContext(GLFWwindow *window);

    [[nodiscard]] const vk::raii::Instance &GetInstance() const { return *instance; }

    [[nodiscard]] vk::raii::Device &getDevice() { return *device; }

    [[nodiscard]] vk::raii::PhysicalDevice &getPhysicalDevice() { return *physicalDevice; }

    [[nodiscard]] vk::raii::SurfaceKHR &getSurface() { return *surface; }

    [[nodiscard]] vk::raii::Queue &getGraphicsQueue() { return *graphicsQueue; }

    [[nodiscard]] uint32_t getGraphicsQueueFamily() const { return graphicsQueueFamily; }

    vk::raii::SwapchainKHR &getSwapChain() { return *swapchain; }

    std::vector<vk::Image> getSwapChainImages() {
        return swapchainImages;
    }

    std::vector<vk::ImageView> getSwapChainImageViews() {
        return swapchainImageViews;
    }
};
