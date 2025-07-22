#pragma once
#include "../Resource/ShaderManager.h"
#include "GLFW/glfw3.h"
#include "RenderContext.h"


class Renderer {
    RenderContext renderContext;
    ShaderManager shaderManager;

    vk::PipelineLayout pipelineLayout;
    vk::RenderPass renderPass;
    vk::raii::Pipeline forwardPipeline = nullptr;
    vk::raii::Pipeline uiPipeline = nullptr;


public:
    explicit Renderer(GLFWwindow *window) : renderContext{window} {
        shaderManager = {};
        shaderManager.compile();
        createRenderPass();
        createGraphicsPipeline();
    }

    void createGraphicsPipeline() {
        auto forwardShaderStages = createShaderStages("Forward");

        const std::vector dynamicStates = {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor
        };

        vk::PipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = vk::StructureType::ePipelineDynamicStateCreateInfo;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = vk::StructureType::ePipelineVertexInputStateCreateInfo;
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.pVertexBindingDescriptions = nullptr;
        vertexInputInfo.vertexAttributeDescriptionCount = 0;
        vertexInputInfo.pVertexAttributeDescriptions = nullptr;

        vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = vk::StructureType::ePipelineInputAssemblyStateCreateInfo;
        inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        vk::Viewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = 800.0f;
        viewport.height = 600.0f;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        vk::Rect2D scissor{};
        scissor.offset = vk::Offset2D{0, 0};
        scissor.extent = vk::Extent2D{800, 600};

        vk::PipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = vk::StructureType::ePipelineViewportStateCreateInfo;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        vk::PipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = vk::StructureType::ePipelineRasterizationStateCreateInfo;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = vk::PolygonMode::eFill;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = vk::CullModeFlagBits::eBack;
        rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f;
        rasterizer.depthBiasClamp = 0.0f;
        rasterizer.depthBiasSlopeFactor = 0.0f;
        rasterizer.depthClampEnable = VK_FALSE;

        vk::PipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = vk::StructureType::ePipelineMultisampleStateCreateInfo;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
        multisampling.minSampleShading = 1.0f;
        multisampling.pSampleMask = nullptr;
        multisampling.alphaToCoverageEnable = VK_FALSE;
        multisampling.alphaToOneEnable = VK_FALSE;

        vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                              vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        colorBlendAttachment.blendEnable = VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eOne;
        colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eZero;
        colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
        colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
        colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
        colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;

        vk::PipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = vk::StructureType::ePipelineColorBlendStateCreateInfo;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = vk::LogicOp::eCopy;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = vk::StructureType::ePipelineLayoutCreateInfo;
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pSetLayouts = nullptr;

        pipelineLayout = renderContext.getDevice().createPipelineLayout(pipelineLayoutInfo);

        vk::GraphicsPipelineCreateInfo forwardPipelineInfo{};
        forwardPipelineInfo.sType = vk::StructureType::eGraphicsPipelineCreateInfo;
        forwardPipelineInfo.stageCount = static_cast<uint32_t>(forwardShaderStages.size());
        forwardPipelineInfo.pStages = forwardShaderStages.data();
        forwardPipelineInfo.pVertexInputState = &vertexInputInfo;
        forwardPipelineInfo.pInputAssemblyState = &inputAssembly;
        forwardPipelineInfo.pViewportState = &viewportState;
        forwardPipelineInfo.pRasterizationState = &rasterizer;
        forwardPipelineInfo.pMultisampleState = &multisampling;
        forwardPipelineInfo.pDepthStencilState = nullptr;
        forwardPipelineInfo.pColorBlendState = &colorBlending;
        forwardPipelineInfo.pDynamicState = &dynamicState;
        forwardPipelineInfo.layout = pipelineLayout;
        forwardPipelineInfo.renderPass = renderPass;
        forwardPipelineInfo.subpass = 0;
        forwardPipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        forwardPipelineInfo.basePipelineIndex = -1;

        vk::GraphicsPipelineCreateInfo uiPipelineInfo{};
        uiPipelineInfo.sType = vk::StructureType::eGraphicsPipelineCreateInfo;
        uiPipelineInfo.stageCount = 0;
        uiPipelineInfo.pVertexInputState = &vertexInputInfo;
        uiPipelineInfo.pInputAssemblyState = &inputAssembly;
        uiPipelineInfo.pViewportState = &viewportState;
        uiPipelineInfo.pRasterizationState = &rasterizer;
        uiPipelineInfo.pMultisampleState = &multisampling;
        uiPipelineInfo.pDepthStencilState = nullptr;
        uiPipelineInfo.pColorBlendState = &colorBlending;
        uiPipelineInfo.pDynamicState = &dynamicState;
        uiPipelineInfo.layout = pipelineLayout;
        uiPipelineInfo.renderPass = renderPass;
        uiPipelineInfo.subpass = 1;
        uiPipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        uiPipelineInfo.basePipelineIndex = -1;

        forwardPipeline = {renderContext.getDevice(), nullptr, forwardPipelineInfo};
        uiPipeline = {renderContext.getDevice(), nullptr, uiPipelineInfo};
    }

    void createRenderPass() {
        vk::AttachmentDescription colorAttachment{};
        colorAttachment.format = vk::Format::eB8G8R8A8Srgb;
        colorAttachment.samples = vk::SampleCountFlagBits::e1;
        colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
        colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
        colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
        colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

        vk::AttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

        vk::SubpassDescription forwardSubpass{};
        forwardSubpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
        forwardSubpass.colorAttachmentCount = 1;
        forwardSubpass.pColorAttachments = &colorAttachmentRef;

        vk::SubpassDescription uiSubpass{};
        uiSubpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
        uiSubpass.colorAttachmentCount = 1;
        uiSubpass.pColorAttachments = &colorAttachmentRef;

        const vk::SubpassDescription subpasses[] = {forwardSubpass, uiSubpass};

        vk::SubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dependency.srcAccessMask = vk::AccessFlagBits::eNone;
        dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

        vk::RenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = vk::StructureType::eRenderPassCreateInfo;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 2;
        renderPassInfo.pSubpasses = subpasses;
        renderPassInfo.pDependencies = &dependency;

        renderPass = renderContext.getDevice().createRenderPass(renderPassInfo);
    }

    void render() {

    }
private:
    std::vector<vk::PipelineShaderStageCreateInfo> createShaderStages(const char* module) {
        auto spirvCodePairOpt = shaderManager.getSpirvCode(module);
        if (!spirvCodePairOpt) {
            throw std::runtime_error("Failed to get SPIR-V code for module: " + std::string(module));
        }
        const auto& [vertSpirv, fragSpirv] = *spirvCodePairOpt;

        auto vertShaderStage = createShaderStage(vertSpirv);
        auto fragShaderStage = createShaderStage(fragSpirv, vk::ShaderStageFlagBits::eFragment);

        return {vertShaderStage, fragShaderStage};
    }

    vk::PipelineShaderStageCreateInfo createShaderStage(const Slang::ComPtr<ISlangBlob> &spirvCode,
                                                        vk::ShaderStageFlagBits stage = vk::ShaderStageFlagBits::eVertex) {
        vk::ShaderModuleCreateInfo shaderModuleCreateInfo{};
        shaderModuleCreateInfo.sType = vk::StructureType::eShaderModuleCreateInfo;
        shaderModuleCreateInfo.codeSize = spirvCode->getBufferSize();
        shaderModuleCreateInfo.pCode = static_cast<const uint32_t *>(spirvCode->getBufferPointer());
        const auto forwardVertShaderModule = renderContext.getDevice().createShaderModule(shaderModuleCreateInfo);

        vk::PipelineShaderStageCreateInfo shaderStageInfo{};
        shaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
        shaderStageInfo.stage = stage;
        shaderStageInfo.module = forwardVertShaderModule;
        shaderStageInfo.pName = "vertexMain";

        return shaderStageInfo;
    }
};