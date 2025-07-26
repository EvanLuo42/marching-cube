#pragma once
#include "../Resource/ShaderManager.h"
#include "RenderContext.h"

#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>
#include <backends/imgui_impl_glfw.h>

static void checkVkResult(const VkResult err) {
    if (err == VK_SUCCESS)
        return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
        abort();
}

class Renderer {
    RenderContext renderContext;
    ShaderManager shaderManager;

    vk::raii::RenderPass uiRenderPass = nullptr;
    vk::raii::RenderPass forwardRenderPass = nullptr;

    vk::raii::DescriptorPool uiDescriptorPool = nullptr;
    vk::raii::DescriptorPool forwardDescriptorPool = nullptr;
    vk::raii::DescriptorSetLayout forwardDescriptorSetLayout = nullptr;

    vk::raii::PipelineCache pipelineCache = nullptr;
    vk::raii::PipelineLayout forwardPipelineLayout = nullptr;
    vk::raii::Pipeline forwardPipeline = nullptr;

    vk::raii::Image forwardDepthImage = nullptr;
    vk::raii::DeviceMemory forwardDepthMemory = nullptr;
    vk::raii::ImageView forwardDepthImageView = nullptr;

    std::vector<vk::raii::CommandBuffer> uiCommandBuffers;
    std::vector<vk::raii::CommandBuffer> forwardCommandBuffers;

    std::vector<vk::raii::Framebuffer> forwardFrameBuffers;
    std::vector<vk::raii::Framebuffer> uiFrameBuffers;

    std::vector<vk::raii::Semaphore> imageAvailableSemaphores;
    std::vector<vk::raii::Semaphore> forwardFinishedSemaphores;
    std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
    std::vector<vk::raii::Fence> inFlightFences;

    size_t currentFrame = 0;
    const int maxFramesInFlight = 2;

    vk::Extent2D swapchainExtent;
    uint32_t currentImageIndex = 0;

public:
    explicit Renderer(GLFWwindow *window) : renderContext{window} {
        shaderManager.compile();

        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        (void) io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

        swapchainExtent = renderContext.surfaceData.value().extent;

        initDepthResources();
        initDescriptorPools();
        initRenderPasses();
        initPipelineLayout();
        initRenderPipelines();

        initFrameBuffers();
        initSemaphoresAndFences();
        initCommandBuffers();

        initImGui(window);
    }

    ~Renderer() {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    void beginFrame();

    void renderScene();

    void renderUI();

    void endFrame();

private:
    void initRenderPasses();
    void initDescriptorPools();
    void initFrameBuffers();
    void initDepthResources();
    void initCommandBuffers();
    void initSemaphoresAndFences();
    void initPipelineLayout();
    void initRenderPipelines();
    void initImGui(GLFWwindow* window) const;
};
