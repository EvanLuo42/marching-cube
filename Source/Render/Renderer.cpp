#include "Renderer.h"

void Renderer::beginFrame() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void Renderer::renderScene() {
    renderContext.device.waitForFences(*inFlightFences[currentImageIndex], true, UINT64_MAX);
    renderContext.device.resetFences(*inFlightFences[currentImageIndex]);

    auto [_, imageIndex] = renderContext.swapChainData->swapChain.acquireNextImage(
            INT_MAX, *imageAvailableSemaphores[currentImageIndex]);

    const vk::raii::CommandBuffer &cmd = forwardCommandBuffers[imageIndex];
    cmd.reset();
    cmd.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

    std::array<vk::ClearValue, 2> clearValues;
    clearValues[0].color = vk::ClearColorValue(0.0f, 0.623f, 0.717f, 1.0f);
    clearValues[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

    const vk::RenderPassBeginInfo renderPassBeginInfo{
            forwardRenderPass, forwardFrameBuffers[currentImageIndex],
            vk::Rect2D{vk::Offset2D(0, 0), renderContext.surfaceData.value().extent}, clearValues};

    cmd.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

    // Clear + Begin RenderPass + Bind + Draw

    cmd.endRenderPass();
    cmd.end();

    vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    const vk::SubmitInfo submitInfo = {*imageAvailableSemaphores[imageIndex], waitStage, *cmd,
                                       *forwardFinishedSemaphores[imageIndex]};

    renderContext.graphicsQueue.submit(submitInfo);
    currentImageIndex = imageIndex;
}

void Renderer::renderUI() {
    ImGui::Render();
    ImDrawData *drawData = ImGui::GetDrawData();

    const vk::raii::CommandBuffer &cmd = uiCommandBuffers[currentImageIndex];
    cmd.reset();
    cmd.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

    const vk::RenderPassBeginInfo beginInfo = {
            *uiRenderPass, *uiFrameBuffers[currentImageIndex], {{0, 0}, swapchainExtent}, 0, nullptr};

    cmd.beginRenderPass(beginInfo, vk::SubpassContents::eInline);
    ImGui_ImplVulkan_RenderDrawData(drawData, *cmd);
    cmd.endRenderPass();

    cmd.end();

    vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    const vk::SubmitInfo submitInfo = {*forwardFinishedSemaphores[currentImageIndex], waitStage, *cmd,
                                       *renderFinishedSemaphores[currentImageIndex]};

    renderContext.graphicsQueue.submit(submitInfo, *inFlightFences[currentImageIndex]);
}

void Renderer::endFrame() {
    const vk::PresentInfoKHR presentInfo = {*renderFinishedSemaphores[currentImageIndex],
                                            *renderContext.swapChainData->swapChain, currentImageIndex};

    renderContext.presentQueue.presentKHR(presentInfo);

    currentImageIndex = (currentImageIndex + 1) % imageAvailableSemaphores.size();
}

void Renderer::initRenderPasses() {
    const vk::Format colorFormat = vk::su::pickSurfaceFormat(renderContext.physicalDevice.getSurfaceFormatsKHR(
                                                                     renderContext.surfaceData->surface))
                                           .format;
    forwardRenderPass = vk::raii::su::makeRenderPass(renderContext.device, colorFormat, vk::Format::eD32Sfloat,
                                                     vk::AttachmentLoadOp::eClear, vk::ImageLayout::eUndefined,
                                                     vk::ImageLayout::ePresentSrcKHR);
    uiRenderPass = vk::raii::su::makeRenderPass(renderContext.device, colorFormat, vk::Format::eUndefined,
                                                vk::AttachmentLoadOp::eLoad, vk::ImageLayout::ePresentSrcKHR,
                                                vk::ImageLayout::ePresentSrcKHR);
}

void Renderer::initDescriptorPools() {
    uiDescriptorPool = vk::raii::su::makeDescriptorPool(renderContext.device,
                                                        {
                                                                {vk::DescriptorType::eUniformBuffer, 10},
                                                                {vk::DescriptorType::eCombinedImageSampler, 1000},
                                                                {vk::DescriptorType::eSampler, 1000},
                                                                {vk::DescriptorType::eSampledImage, 1000},
                                                        });

    forwardDescriptorPool = vk::raii::su::makeDescriptorPool(renderContext.device,
                                                             {
                                                                     {vk::DescriptorType::eUniformBuffer, 100},
                                                                     {vk::DescriptorType::eCombinedImageSampler, 100},
                                                             });
}

void Renderer::initFrameBuffers() {
    for (const auto &imageView: renderContext.swapChainData->imageViews) {
        vk::ImageView attachments[] = {*imageView, *forwardDepthImageView};
        vk::FramebufferCreateInfo fbInfo({}, *forwardRenderPass, 2, attachments, swapchainExtent.width,
                                         swapchainExtent.height, 1);
        forwardFrameBuffers.emplace_back(renderContext.device, fbInfo);
    }

    for (const auto &imageView: renderContext.swapChainData->imageViews) {
        vk::FramebufferCreateInfo fbInfo = {
                {}, *uiRenderPass, 1, &*imageView, swapchainExtent.width, swapchainExtent.height, 1};
        uiFrameBuffers.emplace_back(renderContext.device, fbInfo);
    }
}

void Renderer::initDepthResources() {
    constexpr auto depthFormat = vk::Format::eD32Sfloat;

    vk::ImageCreateInfo imageInfo(
            {}, vk::ImageType::e2D, depthFormat, vk::Extent3D{swapchainExtent.width, swapchainExtent.height, 1}, 1, 1,
            vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment);

    forwardDepthImage = {renderContext.device, imageInfo};
    const vk::MemoryRequirements memRequirements = forwardDepthImage.getMemoryRequirements();

    vk::MemoryAllocateInfo allocInfo(
            memRequirements.size, vk::su::findMemoryType(renderContext.physicalDevice, memRequirements.memoryTypeBits,
                                                         vk::MemoryPropertyFlagBits::eDeviceLocal));
    forwardDepthMemory = {renderContext.device, allocInfo};
    forwardDepthImage.bindMemory(*forwardDepthMemory, 0);

    vk::ImageViewCreateInfo viewInfo({}, *forwardDepthImage, vk::ImageViewType::e2D, depthFormat, {},
                                     {vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1});
    forwardDepthImageView = {renderContext.device, viewInfo};
}

void Renderer::initCommandBuffers() {
    uiCommandBuffers = renderContext.device.allocateCommandBuffers(vk::CommandBufferAllocateInfo{
            renderContext.commandPool, vk::CommandBufferLevel::ePrimary,
            static_cast<uint32_t>(renderContext.swapChainData->swapChain.getImages().size())});
    forwardCommandBuffers = renderContext.device.allocateCommandBuffers(vk::CommandBufferAllocateInfo{
            renderContext.commandPool, vk::CommandBufferLevel::ePrimary,
            static_cast<uint32_t>(renderContext.swapChainData->swapChain.getImages().size())});
}

void Renderer::initSemaphoresAndFences() {
    const size_t imageCount = renderContext.swapChainData->imageViews.size();
    imageAvailableSemaphores.reserve(imageCount);
    renderFinishedSemaphores.reserve(imageCount);
    inFlightFences.reserve(imageCount);

    for (size_t i = 0; i < imageCount; ++i) {
        imageAvailableSemaphores.emplace_back(renderContext.device, vk::SemaphoreCreateInfo{});
        forwardFinishedSemaphores.emplace_back(renderContext.device, vk::SemaphoreCreateInfo{});
        renderFinishedSemaphores.emplace_back(renderContext.device, vk::SemaphoreCreateInfo{});
        inFlightFences.emplace_back(renderContext.device, vk::FenceCreateInfo{vk::FenceCreateFlagBits::eSignaled});
    }
}

void Renderer::initPipelineLayout() {
    forwardDescriptorSetLayout = vk::raii::su::makeDescriptorSetLayout(
            renderContext.device, {{vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex}});
    forwardPipelineLayout = {renderContext.device, {{}, *forwardDescriptorSetLayout}};
}

void Renderer::initRenderPipelines() {
    pipelineCache = {renderContext.device, vk::PipelineCacheCreateInfo{}};
    auto spirv = shaderManager.getSpirvCode("DefaultLit");
    if (!spirv) {
        throw std::runtime_error("Missing shader: DefaultLit");
    }

    const auto &[vert, frag] = *spirv;

    const vk::ShaderModuleCreateInfo vsInfo({}, vert->getBufferSize(),
                                            static_cast<const uint32_t *>(vert->getBufferPointer()));
    const vk::ShaderModuleCreateInfo fsInfo({}, frag->getBufferSize(),
                                            static_cast<const uint32_t *>(frag->getBufferPointer()));
    const vk::raii::ShaderModule vertModule(renderContext.device, vsInfo);
    const vk::raii::ShaderModule fragModule(renderContext.device, fsInfo);

    forwardPipeline = vk::raii::su::makeGraphicsPipeline(renderContext.device, pipelineCache, vertModule, fragModule,
                                                         sizeof(float) * 8,
                                                         {
                                                                 {0, 0, vk::Format::eR32G32B32Sfloat, 0}, // position
                                                                 {1, 0, vk::Format::eR32G32Sfloat, 12}, // uv
                                                                 {2, 0, vk::Format::eR32G32B32Sfloat, 20} // normal
                                                         },
                                                         forwardPipelineLayout, forwardRenderPass, true);
}

void Renderer::initImGui(GLFWwindow *window) const {
    ImGui_ImplGlfw_InitForVulkan(window, true);
    ImGui_ImplVulkan_InitInfo initInfo{};
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
