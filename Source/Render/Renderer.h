#pragma once
#include "../Resource/ShaderManager.h"
#include "RenderContext.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>

#include "Camera.h"
#include "RenderSettings.h"
#include "Vertex.h"

class Renderer {
    RenderContext renderContext;
    ShaderManager shaderManager;

    Camera camera;

    vk::raii::RenderPass uiRenderPass = nullptr;
    vk::raii::RenderPass forwardRenderPass = nullptr;

    vk::raii::DescriptorPool uiDescriptorPool = nullptr;
    vk::raii::DescriptorPool forwardDescriptorPool = nullptr;
    vk::raii::DescriptorSetLayout forwardDescriptorSetLayout = nullptr;
    vk::raii::DescriptorSet forwardDescriptorSet = nullptr;

    vk::raii::PipelineCache pipelineCache = nullptr;
    vk::raii::PipelineLayout forwardPipelineLayout = nullptr;
    vk::raii::Pipeline forwardPipeline = nullptr;

    std::optional<vk::raii::su::DepthBufferData> forwardDepthBuffer;

    std::vector<vk::raii::CommandBuffer> uiCommandBuffers;
    std::vector<vk::raii::CommandBuffer> forwardCommandBuffers;

    std::vector<vk::raii::Framebuffer> forwardFrameBuffers;
    std::vector<vk::raii::Framebuffer> uiFrameBuffers;

    std::vector<vk::raii::Semaphore> imageAvailableSemaphores;
    std::vector<vk::raii::Semaphore> forwardFinishedSemaphores;
    std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
    std::vector<vk::raii::Fence> inFlightFences;

    std::optional<vk::raii::su::BufferData> vertexBuffer;
    std::optional<vk::raii::su::BufferData> indexBuffer;
    std::optional<vk::raii::su::BufferData> uniformBuffer;

    size_t currentFrame = 0;
    const int maxFramesInFlight = 2;

    vk::Extent2D swapchainExtent;
    uint32_t currentImageIndex = 0;

public:
    explicit Renderer(GLFWwindow *window) : renderContext{window}, camera{window} {
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
        initBuffers();

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

    void renderScene(const RenderSettings &renderSettings) const;

    void renderUI();

    void endFrame();

    void cameraUpdate(float deltaTime);

    void updateBuffers(const std::vector<Vertex> &vertices, const std::vector<uint16_t> &indices);

private:
    void initRenderPasses();
    void initDescriptorPools();
    void initFrameBuffers();
    void initDepthResources();
    void initCommandBuffers();
    void initSemaphoresAndFences();
    void initPipelineLayout();
    void initRenderPipelines();
    void initImGui(GLFWwindow *window) const;

    void initBuffers();
};
