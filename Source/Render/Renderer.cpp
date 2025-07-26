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

    renderContext.device.waitForFences(*inFlightFences[currentImageIndex], true, UINT64_MAX);
    renderContext.device.resetFences(*inFlightFences[currentImageIndex]);

    auto [_, imageIndex] = renderContext.swapChainData->swapChain.acquireNextImage(INT_MAX, *imageAvailableSemaphores[currentImageIndex]);

    const vk::raii::CommandBuffer& cmd = uiCommandBuffers[imageIndex];
    cmd.reset();
    cmd.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

    vk::ClearValue clearColor{std::array{0.1f, 0.1f, 0.1f, 1.0f}};
    const vk::RenderPassBeginInfo beginInfo = {
        *uiRenderPass,
        *uiFramebuffers[imageIndex],
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
        *imageAvailableSemaphores[imageIndex],
        waitStage,
        *cmd,
        *renderFinishedSemaphores[imageIndex]
    };

    renderContext.graphicsQueue.submit(submitInfo, *inFlightFences[imageIndex]);
    currentImageIndex = imageIndex;
}

void Renderer::endFrame() {
    const vk::PresentInfoKHR presentInfo = {
        *renderFinishedSemaphores[currentImageIndex],
        *renderContext.swapChainData->swapChain,
        currentImageIndex
    };

    renderContext.presentQueue.presentKHR(presentInfo);

    currentImageIndex = (currentImageIndex + 1) % imageAvailableSemaphores.size();
}

void Renderer::initRenderPasses() {
    const vk::Format colorFormat = vk::su::pickSurfaceFormat(renderContext.physicalDevice.getSurfaceFormatsKHR(renderContext.surfaceData->surface)).format;
    uiRenderPass = vk::raii::su::makeRenderPass(renderContext.device, colorFormat, vk::Format::eUndefined);
}

void Renderer::initDescriptorPools() {
    uiDescriptorPool = vk::raii::su::makeDescriptorPool(renderContext.device, {
        { vk::DescriptorType::eUniformBuffer, 1 },
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

    const size_t imageCount = renderContext.swapChainData->imageViews.size();
    imageAvailableSemaphores.reserve(imageCount);
    renderFinishedSemaphores.reserve(imageCount);
    inFlightFences.reserve(imageCount);

    for (size_t i = 0; i < imageCount; ++i) {
        imageAvailableSemaphores.emplace_back(renderContext.device, vk::SemaphoreCreateInfo{});
        renderFinishedSemaphores.emplace_back(renderContext.device, vk::SemaphoreCreateInfo{});
        inFlightFences.emplace_back(renderContext.device, vk::FenceCreateInfo{vk::FenceCreateFlagBits::eSignaled});
    }
}
