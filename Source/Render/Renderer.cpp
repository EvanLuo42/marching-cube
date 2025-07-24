#include "Renderer.h"

void Renderer::beginFrame() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void Renderer::renderScene() {}

void Renderer::renderUI() {
    ImGui::Render();
    ImDrawData* drawData = ImGui::GetDrawData();

    renderContext.device.waitForFences(*inFlightFences[currentFrame], true, UINT64_MAX);
    renderContext.device.resetFences(*inFlightFences[currentFrame]);

    auto [_, index] = renderContext.swapChainData->swapChain.acquireNextImage(INT_MAX, *imageAvailableSemaphores[currentFrame]);

    currentImageIndex = index;

    const vk::raii::CommandBuffer& cmd = uiCommandBuffers[currentImageIndex];
    cmd.reset();
    cmd.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

    vk::ClearValue clearColor{std::array{0.1f, 0.1f, 0.1f, 1.0f}};
    const vk::RenderPassBeginInfo beginInfo = {
        *uiRenderPass,
        *uiFramebuffers[currentImageIndex],
        {{0, 0}, swapchainExtent},
        1,
        &clearColor
    };

    cmd.beginRenderPass(beginInfo, vk::SubpassContents::eInline);
    ImGui_ImplVulkan_RenderDrawData(drawData, *cmd);
    cmd.endRenderPass();
    cmd.end();

    vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    const vk::SubmitInfo submitInfo = {
        *imageAvailableSemaphores[currentFrame],
        waitStage,
        *cmd,
        *renderFinishedSemaphores[currentFrame]
    };

    renderContext.graphicsQueue.submit(submitInfo, *inFlightFences[currentFrame]);
}

void Renderer::endFrame() {
    const vk::PresentInfoKHR presentInfo = {
        *renderFinishedSemaphores[currentFrame],
        *renderContext.swapChainData->swapChain,
        currentImageIndex
    };

    renderContext.presentQueue.presentKHR(presentInfo);

    currentFrame = (currentFrame + 1) % maxFramesInFlight;
}

void Renderer::initRenderPasses() {
    const vk::Format colorFormat = vk::su::pickSurfaceFormat(renderContext.physicalDevice.getSurfaceFormatsKHR(renderContext.surfaceData->surface)).format;
    uiRenderPass = vk::raii::su::makeRenderPass(renderContext.device, colorFormat, vk::Format::eUndefined);
}

void Renderer::initDescriptorPools() {
    uiDescriptorPool = vk::raii::su::makeDescriptorPool(renderContext.device, {
        {vk::DescriptorType::eUniformBuffer, 1},
        { vk::DescriptorType::eCombinedImageSampler, 1000 },
        { vk::DescriptorType::eSampler, 1000 },
        { vk::DescriptorType::eSampledImage, 1000 },
    });
}

void Renderer::initFrameBuffers() {
    swapchainExtent = renderContext.surfaceData.value().extent;

    for (const auto& imageView : renderContext.swapChainData->imageViews) {
        vk::FramebufferCreateInfo fbInfo = {
            {},
            *uiRenderPass,
            1,
            &*imageView,
            swapchainExtent.width,
            swapchainExtent.height,
            1
        };
        uiFramebuffers.emplace_back(renderContext.device, fbInfo);
    }

    for (int i = 0; i < maxFramesInFlight; ++i) {
        imageAvailableSemaphores.emplace_back(renderContext.device, vk::SemaphoreCreateInfo{});
        renderFinishedSemaphores.emplace_back(renderContext.device, vk::SemaphoreCreateInfo{});
        inFlightFences.emplace_back(renderContext.device, vk::FenceCreateInfo{vk::FenceCreateFlagBits::eSignaled});
    }
}
