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

    std::vector<vk::raii::CommandBuffer> uiCommandBuffers;

    std::vector<vk::raii::Framebuffer> uiFramebuffers;
    std::vector<vk::raii::Semaphore> imageAvailableSemaphores;
    std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
    std::vector<vk::raii::Fence> inFlightFences;

    size_t currentFrame = 0;
    const int maxFramesInFlight = 2;

    vk::Extent2D swapchainExtent;
    uint32_t currentImageIndex = 0;
public:
    explicit Renderer(GLFWwindow *window) : renderContext{window} {
        shaderManager.compile();

        uiCommandBuffers = renderContext.device.allocateCommandBuffers(
            vk::CommandBufferAllocateInfo{
                renderContext.commandPool,
                vk::CommandBufferLevel::ePrimary,
                static_cast<uint32_t>(renderContext.swapChainData->swapChain.getImages().size())
            }
        );

        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        (void) io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

        initRenderPasses();
        initDescriptorPools();

        initFrameBuffers();

        ImGui_ImplGlfw_InitForVulkan(window, true);
        ImGui_ImplVulkan_InitInfo initInfo;
        initInfo.Instance = *renderContext.instance;
        initInfo.PhysicalDevice = *renderContext.physicalDevice;
        initInfo.Device = *renderContext.device;
        initInfo.QueueFamily = renderContext.graphicsQueueFamilyIndex;
        initInfo.Queue = *renderContext.graphicsQueue;
        initInfo.PipelineCache = *renderContext.pipelineCache;
        initInfo.DescriptorPool = *uiDescriptorPool;
        initInfo.RenderPass = *uiRenderPass;
        initInfo.Subpass = 0;
        initInfo.MinImageCount = 2;
        initInfo.ImageCount = renderContext.swapChainData.value().swapChain.getImages().size();
        initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        initInfo.CheckVkResultFn = checkVkResult;
        ImGui_ImplVulkan_Init(&initInfo);
    }

    void beginFrame();

    void renderScene();

    void renderUI();

    void endFrame();

private:
    void initRenderPasses();
    void initDescriptorPools();
    void initFrameBuffers();
};
