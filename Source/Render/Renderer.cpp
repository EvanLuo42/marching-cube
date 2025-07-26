#include "Renderer.h"

#include "UniformBufferObject.h"

#include <glm/gtc/matrix_transform.hpp>

#include "Vertex.h"

void Renderer::beginFrame() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    renderContext.device.waitForFences(*inFlightFences[currentImageIndex], true, UINT64_MAX);
    renderContext.device.resetFences(*inFlightFences[currentImageIndex]);

    auto [_, imageIndex] = renderContext.swapChainData->swapChain.acquireNextImage(
            INT_MAX, *imageAvailableSemaphores[currentImageIndex]);
    currentImageIndex = imageIndex;
}

void Renderer::renderScene(const RenderSettings &renderSettings) const {
    UniformBufferObject ubo{};
    ubo.model = glm::scale(glm::identity<glm::mat4>(), glm::vec3(5.0f, 0.5f, 5.0f));
    ubo.view = camera.getViewMatrix();
    const float aspect = static_cast<float>(swapchainExtent.width) /
               static_cast<float>(swapchainExtent.height);

    ubo.proj = glm::perspectiveRH_ZO(glm::radians(45.0f), aspect, 0.1f, 100.0f);
    ubo.proj[1][1] *= -1;
    ubo.cameraPos = camera.position;

    uniformBuffer->upload(ubo);

    const vk::raii::CommandBuffer &cmd = forwardCommandBuffers[currentImageIndex];
    cmd.reset();
    cmd.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

    cmd.pushConstants(
        forwardPipelineLayout,
        vk::ShaderStageFlagBits::eFragment,
        0,
        vk::ArrayProxy<const BlinnPhongVariables>(1, &renderSettings.lighting)
    );

    std::array<vk::ClearValue, 2> clearValues;
    clearValues[0].color = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
    clearValues[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

    const vk::RenderPassBeginInfo renderPassBeginInfo{
            forwardRenderPass, forwardFrameBuffers[currentImageIndex],
            vk::Rect2D{vk::Offset2D(0, 0), renderContext.surfaceData.value().extent}, clearValues};

    cmd.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

    // Clear + Begin RenderPass + Bind + Draw

    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *forwardPipeline);

    const vk::Viewport viewport{
            0.0f, 0.0f, static_cast<float>(swapchainExtent.width), static_cast<float>(swapchainExtent.height),
            0.0f, 1.0f};
    cmd.setViewport(0, viewport);

    const vk::Rect2D scissor{{0, 0}, swapchainExtent};
    cmd.setScissor(0, scissor);

    cmd.bindVertexBuffers(0, {*vertexBuffer->buffer}, {0});
    cmd.bindIndexBuffer(*indexBuffer->buffer, 0, vk::IndexType::eUint16);

    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *forwardPipelineLayout, 0, *forwardDescriptorSet, nullptr);

    cmd.drawIndexed(static_cast<uint32_t>(cubeIndices.size()), 1, 0, 0, 0);

    cmd.endRenderPass();
    cmd.end();

    vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    const vk::SubmitInfo submitInfo = {*imageAvailableSemaphores[currentImageIndex], waitStage, *cmd,
                                       *forwardFinishedSemaphores[currentImageIndex]};

    renderContext.graphicsQueue.submit(submitInfo);
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

void Renderer::cameraUpdate() { camera.update(); }

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
        vk::ImageView attachments[] = {*imageView, *forwardDepthBuffer.value().imageView};
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

    forwardDepthBuffer = vk::raii::su::DepthBufferData(renderContext.physicalDevice, renderContext.device, depthFormat,
                                                       swapchainExtent);
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
        renderContext.device,
        {
            {vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment}
        }
    );

    vk::PushConstantRange blinnPhongRange{
        vk::ShaderStageFlagBits::eFragment,
        0,
        sizeof(glm::vec3) + sizeof(float)
    };
    forwardPipelineLayout = {renderContext.device, {{}, *forwardDescriptorSetLayout, blinnPhongRange}};
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
    initInfo.CheckVkResultFn = nullptr;
    ImGui_ImplVulkan_Init(&initInfo);
}

void Renderer::initBuffers() {
    const auto &pd = renderContext.physicalDevice;
    const auto &dev = renderContext.device;
    const auto &cmdPool = renderContext.commandPool;
    const auto &queue = renderContext.graphicsQueue;

    // vertexBuffer =
    //         vk::raii::su::BufferData(pd, dev, sizeof(Vertex) * cubeVertices.size(),
    //                                  vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
    //                                  vk::MemoryPropertyFlagBits::eDeviceLocal);
    //
    // vertexBuffer->upload(pd, dev, cmdPool, queue, cubeVertices, sizeof(Vertex));
    //
    // indexBuffer =
    //         vk::raii::su::BufferData(pd, dev, sizeof(uint16_t) * cubeIndices.size(),
    //                                  vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
    //                                  vk::MemoryPropertyFlagBits::eDeviceLocal);
    //
    // indexBuffer->upload(pd, dev, cmdPool, queue, cubeIndices, sizeof(uint16_t));

    uniformBuffer = vk::raii::su::BufferData(renderContext.physicalDevice, renderContext.device,
                                             sizeof(UniformBufferObject), vk::BufferUsageFlagBits::eUniformBuffer,
                                             vk::MemoryPropertyFlagBits::eHostVisible |
                                                     vk::MemoryPropertyFlagBits::eHostCoherent);

    const vk::DescriptorSetAllocateInfo allocInfo{*forwardDescriptorPool, 1, &*forwardDescriptorSetLayout};
    forwardDescriptorSet = std::move(renderContext.device.allocateDescriptorSets(allocInfo).front());

    vk::DescriptorBufferInfo bufferInfo{*uniformBuffer->buffer, 0, sizeof(UniformBufferObject)};

    const vk::WriteDescriptorSet write{
            *forwardDescriptorSet, 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &bufferInfo};

    renderContext.device.updateDescriptorSets(write, nullptr);
}
