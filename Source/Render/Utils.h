#pragma once
#include <iostream>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <numeric>

#include <vulkan/vulkan_raii.hpp>

namespace vk::su {
    inline std::vector<std::string> getInstanceExtensions() {
        uint32_t extensionsCount = 0;
        const char **extensions = glfwGetRequiredInstanceExtensions(&extensionsCount);
        std::vector<std::string> requiredExtensions;

        requiredExtensions.reserve(extensionsCount);
        for (uint32_t i = 0; i < extensionsCount; i++) {
            requiredExtensions.emplace_back(extensions[i]);
        }

#if defined(__APPLE__)
        requiredExtensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        requiredExtensions.emplace_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#endif
        return requiredExtensions;
    }

    inline std::vector<std::string> getDeviceExtensions() {
        return {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#if defined(__APPLE__)
                "VK_KHR_portability_subset",
#endif
        };
    }

    inline std::vector<char const *> gatherExtensions(const std::vector<std::string> &extensions,
#if !defined(NDEBUG)
                                                      const std::vector<ExtensionProperties> &extensionProperties
#endif
    ) {
        std::vector<char const *> enabledExtensions;
        enabledExtensions.reserve(extensions.size());
        for (auto const &extension: extensions) {
            assert(std::ranges::any_of(
                    extensionProperties.begin(), extensionProperties.end(),
                    [extension](ExtensionProperties const &ep) { return extension == ep.extensionName; }));
            enabledExtensions.push_back(extension.data());
        }

#if !defined(NDEBUG)
        if (std::ranges::none_of(
                    extensions,
                    [](std::string const &extension) { return extension == VK_EXT_DEBUG_UTILS_EXTENSION_NAME; }) &&
            std::ranges::any_of(extensionProperties, [](ExtensionProperties const &ep) {
                return strcmp(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, ep.extensionName) == 0;
            })) {
            enabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
#endif
        return enabledExtensions;
    }

    inline std::vector<char const *> gatherLayers(std::vector<std::string> const &layers,
#if !defined(NDEBUG)
                                                  std::vector<LayerProperties> const &layerProperties
#endif
    ) {
        std::vector<char const *> enabledLayers;
        enabledLayers.reserve(layers.size());
        for (auto const &layer: layers) {
            assert(std::ranges::any_of(layerProperties.begin(), layerProperties.end(),
                                       [layer](vk::LayerProperties const &lp) { return layer == lp.layerName; }));
            enabledLayers.push_back(layer.data());
        }
#if !defined(NDEBUG)
        if (std::ranges::none_of(layers,
                                 [](std::string const &layer) { return layer == "VK_LAYER_KHRONOS_validation"; }) &&
            std::ranges::any_of(layerProperties, [](LayerProperties const &lp) {
                return strcmp("VK_LAYER_KHRONOS_validation", lp.layerName) == 0;
            })) {
            enabledLayers.push_back("VK_LAYER_KHRONOS_validation");
        }
#endif
        return enabledLayers;
    }

    VKAPI_ATTR inline Bool32 VKAPI_CALL
    debugUtilsMessengerCallback(const DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                const DebugUtilsMessageTypeFlagsEXT messageTypes,
                                const DebugUtilsMessengerCallbackDataEXT *pCallbackData, void * /*pUserData*/) {
#if !defined(NDEBUG)
        switch (static_cast<uint32_t>(pCallbackData->messageIdNumber)) {
            case 0:
                return False;
            case 0x822806fa:
                return False;
            case 0xe8d1a9fe:
                return False;
            default:;
        }
#endif

        std::cerr << to_string(messageSeverity) << ": " << to_string(messageTypes) << ":\n";
        std::cerr << std::string("\t") << "messageIDName   = <" << pCallbackData->pMessageIdName << ">\n";
        std::cerr << std::string("\t") << "messageIdNumber = " << pCallbackData->messageIdNumber << "\n";
        std::cerr << std::string("\t") << "message         = <" << pCallbackData->pMessage << ">\n";
        if (0 < pCallbackData->queueLabelCount) {
            std::cerr << std::string("\t") << "Queue Labels:\n";
            for (uint32_t i = 0; i < pCallbackData->queueLabelCount; i++) {
                std::cerr << std::string("\t\t") << "labelName = <" << pCallbackData->pQueueLabels[i].pLabelName
                          << ">\n";
            }
        }
        if (0 < pCallbackData->cmdBufLabelCount) {
            std::cerr << std::string("\t") << "CommandBuffer Labels:\n";
            for (uint32_t i = 0; i < pCallbackData->cmdBufLabelCount; i++) {
                std::cerr << std::string("\t\t") << "labelName = <" << pCallbackData->pCmdBufLabels[i].pLabelName
                          << ">\n";
            }
        }
        if (0 < pCallbackData->objectCount) {
            std::cerr << std::string("\t") << "Objects:\n";
            for (uint32_t i = 0; i < pCallbackData->objectCount; i++) {
                std::cerr << std::string("\t\t") << "Object " << i << "\n";
                std::cerr << std::string("\t\t\t")
                          << "objectType   = " << vk::to_string(pCallbackData->pObjects[i].objectType) << "\n";
                std::cerr << std::string("\t\t\t") << "objectHandle = " << pCallbackData->pObjects[i].objectHandle
                          << "\n";
                if (pCallbackData->pObjects[i].pObjectName) {
                    std::cerr << std::string("\t\t\t") << "objectName   = <" << pCallbackData->pObjects[i].pObjectName
                              << ">\n";
                }
            }
        }
        return False;
    }

#if defined(NDEBUG)
    StructureChain<InstanceCreateInfo>
#else
    inline StructureChain<InstanceCreateInfo, DebugUtilsMessengerCreateInfoEXT>
#endif
    makeInstanceCreateInfoChain(InstanceCreateFlagBits instanceCreateFlagBits, ApplicationInfo const &applicationInfo,
                                std::vector<char const *> const &layers, std::vector<char const *> const &extensions) {
#if defined(NDEBUG)
        StructureChain<InstanceCreateInfo> instanceCreateInfo(
                {instanceCreateFlagBits, &applicationInfo, layers, extensions});
#else
        DebugUtilsMessageSeverityFlagsEXT severityFlags(DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                                                        DebugUtilsMessageSeverityFlagBitsEXT::eError);
        DebugUtilsMessageTypeFlagsEXT messageTypeFlags(DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                                                       DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                                                       DebugUtilsMessageTypeFlagBitsEXT::eValidation);
        StructureChain<InstanceCreateInfo, DebugUtilsMessengerCreateInfoEXT> instanceCreateInfo(
                {instanceCreateFlagBits, &applicationInfo, layers, extensions},
                {{}, severityFlags, messageTypeFlags, &debugUtilsMessengerCallback});
#endif
        return instanceCreateInfo;
    }

    inline DebugUtilsMessengerCreateInfoEXT makeDebugUtilsMessengerCreateInfoEXT() {
        return {{},
                DebugUtilsMessageSeverityFlagBitsEXT::eWarning | DebugUtilsMessageSeverityFlagBitsEXT::eError,
                DebugUtilsMessageTypeFlagBitsEXT::eGeneral | DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                        DebugUtilsMessageTypeFlagBitsEXT::eValidation,
                &debugUtilsMessengerCallback};
    }

    inline uint32_t findGraphicsQueueFamilyIndex(std::vector<QueueFamilyProperties> const &queueFamilyProperties) {
        const auto graphicsQueueFamilyProperty = std::find_if(
                queueFamilyProperties.begin(), queueFamilyProperties.end(),
                [](QueueFamilyProperties const &qfp) { return qfp.queueFlags & QueueFlagBits::eGraphics; });
        assert(graphicsQueueFamilyProperty != queueFamilyProperties.end());
        return static_cast<uint32_t>(std::distance(queueFamilyProperties.begin(), graphicsQueueFamilyProperty));
    }

    inline SurfaceFormatKHR pickSurfaceFormat(std::vector<SurfaceFormatKHR> const &formats) {
        assert(!formats.empty());
        SurfaceFormatKHR pickedFormat = formats[0];
        if (formats.size() == 1) {
            if (formats[0].format == Format::eUndefined) {
                pickedFormat.format = Format::eB8G8R8A8Unorm;
                pickedFormat.colorSpace = ColorSpaceKHR::eSrgbNonlinear;
            }
        } else {
            // request several formats, the first found will be used
            constexpr Format requestedFormats[] = {Format::eB8G8R8A8Unorm, Format::eR8G8B8A8Unorm, Format::eB8G8R8Unorm,
                                                   Format::eR8G8B8Unorm};
            auto requestedColorSpace = ColorSpaceKHR::eSrgbNonlinear;
            for (size_t i = 0; i < std::size(requestedFormats); i++) {
                Format requestedFormat = requestedFormats[i];
                auto it = std::ranges::find_if(
                        formats, [requestedFormat, requestedColorSpace](SurfaceFormatKHR const &f) {
                            return f.format == requestedFormat && f.colorSpace == requestedColorSpace;
                        });
                if (it != formats.end()) {
                    pickedFormat = *it;
                    break;
                }
            }
        }
        assert(pickedFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear);
        return pickedFormat;
    }

    template<class T>
    constexpr const T &clamp(const T &v, const T &lo, const T &hi) {
        return v < lo ? lo : hi < v ? hi : v;
    }

    inline uint32_t findMemoryType(const PhysicalDevice physicalDevice, const uint32_t typeFilter,
                                   const MemoryPropertyFlags properties) {
        const PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
            if (typeFilter & 1 << i && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("Failed to find suitable memory type!");
    }

    PresentModeKHR pickPresentMode(std::vector<PresentModeKHR> const &presentModes);
} // namespace vk::su

namespace vk::raii::su {
    struct SurfaceData {
        SurfaceData(Instance const &instance, Extent2D const &extent, GLFWwindow *window) :
            extent(extent), window{window} {
            VkSurfaceKHR _surface;
            if (const VkResult err = glfwCreateWindowSurface(*instance, window, nullptr, &_surface); err != VK_SUCCESS)
                throw std::runtime_error("Failed to create window!");
            surface = SurfaceKHR(instance, _surface);
        }

        Extent2D extent;
        GLFWwindow *window;
        SurfaceKHR surface = nullptr;
    };

    struct SwapChainData {
        SwapChainData(PhysicalDevice const &physicalDevice, Device const &device, SurfaceKHR const &surface,
                      Extent2D const &extent, ImageUsageFlags usage, SwapchainKHR const *pOldSwapchain,
                      uint32_t graphicsQueueFamilyIndex, uint32_t presentQueueFamilyIndex) {
            SurfaceFormatKHR surfaceFormat = vk::su::pickSurfaceFormat(physicalDevice.getSurfaceFormatsKHR(surface));
            colorFormat = surfaceFormat.format;

            SurfaceCapabilitiesKHR surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
            Extent2D swapchainExtent;
            if (surfaceCapabilities.currentExtent.width == (std::numeric_limits<uint32_t>::max)()) {
                swapchainExtent.width = vk::su::clamp(extent.width, surfaceCapabilities.minImageExtent.width,
                                                      surfaceCapabilities.maxImageExtent.width);
                swapchainExtent.height = vk::su::clamp(extent.height, surfaceCapabilities.minImageExtent.height,
                                                       surfaceCapabilities.maxImageExtent.height);
            } else {
                swapchainExtent = surfaceCapabilities.currentExtent;
            }
            SurfaceTransformFlagBitsKHR preTransform =
                    surfaceCapabilities.supportedTransforms & SurfaceTransformFlagBitsKHR::eIdentity
                            ? SurfaceTransformFlagBitsKHR::eIdentity
                            : surfaceCapabilities.currentTransform;
            CompositeAlphaFlagBitsKHR compositeAlpha =
                    surfaceCapabilities.supportedCompositeAlpha & CompositeAlphaFlagBitsKHR::ePreMultiplied
                            ? CompositeAlphaFlagBitsKHR::ePreMultiplied
                    : surfaceCapabilities.supportedCompositeAlpha & CompositeAlphaFlagBitsKHR::ePostMultiplied
                            ? CompositeAlphaFlagBitsKHR::ePostMultiplied
                    : surfaceCapabilities.supportedCompositeAlpha & CompositeAlphaFlagBitsKHR::eInherit
                            ? CompositeAlphaFlagBitsKHR::eInherit
                            : CompositeAlphaFlagBitsKHR::eOpaque;
            PresentModeKHR presentMode = vk::su::pickPresentMode(physicalDevice.getSurfacePresentModesKHR(surface));
            SwapchainCreateInfoKHR swapChainCreateInfo(
                    {}, surface,
                    vk::su::clamp(3u, surfaceCapabilities.minImageCount, surfaceCapabilities.maxImageCount),
                    colorFormat, surfaceFormat.colorSpace, swapchainExtent, 1, usage, vk::SharingMode::eExclusive, {},
                    preTransform, compositeAlpha, presentMode, true, pOldSwapchain ? **pOldSwapchain : nullptr);
            if (graphicsQueueFamilyIndex != presentQueueFamilyIndex) {
                uint32_t queueFamilyIndices[2] = {graphicsQueueFamilyIndex, presentQueueFamilyIndex};
                swapChainCreateInfo.imageSharingMode = SharingMode::eConcurrent;
                swapChainCreateInfo.queueFamilyIndexCount = 2;
                swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
            }
            swapChain = SwapchainKHR(device, swapChainCreateInfo);

            images = swapChain.getImages();

            imageViews.reserve(images.size());
            ImageViewCreateInfo imageViewCreateInfo({}, {}, ImageViewType::e2D, colorFormat, {},
                                                    {ImageAspectFlagBits::eColor, 0, 1, 0, 1});
            for (auto image: images) {
                imageViewCreateInfo.image = image;
                imageViews.emplace_back(device, imageViewCreateInfo);
            }
        }

        Format colorFormat;
        SwapchainKHR swapChain = nullptr;
        std::vector<vk::Image> images;
        std::vector<ImageView> imageViews;
    };

    Instance makeInstance(const Context &context, const std::string &appName, const std::string &engineName,
                          const std::vector<std::string> &layers = {}, const std::vector<std::string> &extensions = {},
                          uint32_t apiVersion = VK_API_VERSION_1_3);

    inline std::pair<uint32_t, uint32_t> findGraphicsAndPresentQueueFamilyIndex(PhysicalDevice const &physicalDevice,
                                                                                SurfaceKHR const &surface) {
        const std::vector<QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
        assert(queueFamilyProperties.size() < (std::numeric_limits<uint32_t>::max)());

        uint32_t graphicsQueueFamilyIndex = vk::su::findGraphicsQueueFamilyIndex(queueFamilyProperties);
        if (physicalDevice.getSurfaceSupportKHR(graphicsQueueFamilyIndex, surface)) {
            return std::make_pair(graphicsQueueFamilyIndex, graphicsQueueFamilyIndex);
        }

        for (size_t i = 0; i < queueFamilyProperties.size(); i++) {
            if (queueFamilyProperties[i].queueFlags & QueueFlagBits::eGraphics &&
                physicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i), surface)) {
                return std::make_pair(static_cast<uint32_t>(i), static_cast<uint32_t>(i));
            }
        }

        for (size_t i = 0; i < queueFamilyProperties.size(); i++) {
            if (physicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i), surface)) {
                return std::make_pair(graphicsQueueFamilyIndex, static_cast<uint32_t>(i));
            }
        }

        throw std::runtime_error("Could not find queues for both graphics or present -> terminating");
    }

    inline Device makeDevice(PhysicalDevice const &physicalDevice, const uint32_t queueFamilyIndex,
                             std::vector<std::string> const &extensions = {},
                             PhysicalDeviceFeatures const *physicalDeviceFeatures = nullptr,
                             void const *pNext = nullptr) {
        std::vector<char const *> enabledExtensions;
        enabledExtensions.reserve(extensions.size());
        for (auto const &ext: extensions) {
            enabledExtensions.push_back(ext.data());
        }

        constexpr float queuePriority = 0.0f;
        DeviceQueueCreateInfo deviceQueueCreateInfo(DeviceQueueCreateFlags(), queueFamilyIndex, 1, &queuePriority);
        const DeviceCreateInfo deviceCreateInfo(DeviceCreateFlags(), deviceQueueCreateInfo, {}, enabledExtensions,
                                                physicalDeviceFeatures, pNext);
        return Device(physicalDevice, deviceCreateInfo);
    }

    inline DescriptorPool makeDescriptorPool(Device const &device, std::vector<DescriptorPoolSize> const &poolSizes) {
        assert(!poolSizes.empty());
        const uint32_t maxSets = std::accumulate(
                poolSizes.begin(), poolSizes.end(), 0,
                [](const uint32_t sum, DescriptorPoolSize const &dps) { return sum + dps.descriptorCount; });
        assert(0 < maxSets);

        const DescriptorPoolCreateInfo descriptorPoolCreateInfo(DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
                                                                maxSets, poolSizes);
        return DescriptorPool(device, descriptorPoolCreateInfo);
    }

    inline RenderPass makeRenderPass(Device const &device, Format colorFormat, Format depthFormat,
                                     AttachmentLoadOp loadOp = AttachmentLoadOp::eClear,
                                     ImageLayout colorInitialLayout = ImageLayout::eUndefined,
                                     ImageLayout colorFinalLayout = ImageLayout::ePresentSrcKHR) {
        std::vector<AttachmentDescription> attachmentDescriptions;
        assert(colorFormat != vk::Format::eUndefined);

        attachmentDescriptions.emplace_back(AttachmentDescriptionFlags(), colorFormat, SampleCountFlagBits::e1, loadOp,
                                            AttachmentStoreOp::eStore, AttachmentLoadOp::eDontCare,
                                            AttachmentStoreOp::eDontCare, colorInitialLayout, colorFinalLayout);
        if (depthFormat != Format::eUndefined) {
            attachmentDescriptions.emplace_back(AttachmentDescriptionFlags(), depthFormat, SampleCountFlagBits::e1,
                                                loadOp, AttachmentStoreOp::eDontCare, AttachmentLoadOp::eDontCare,
                                                AttachmentStoreOp::eDontCare, ImageLayout::eUndefined,
                                                ImageLayout::eDepthStencilAttachmentOptimal);
        }
        AttachmentReference colorAttachment(0, ImageLayout::eColorAttachmentOptimal);
        constexpr AttachmentReference depthAttachment(1, ImageLayout::eDepthStencilAttachmentOptimal);
        SubpassDescription subpassDescription(SubpassDescriptionFlags(), PipelineBindPoint::eGraphics, {},
                                              colorAttachment, {},
                                              depthFormat != Format::eUndefined ? &depthAttachment : nullptr);
        const RenderPassCreateInfo renderPassCreateInfo(RenderPassCreateFlags(), attachmentDescriptions,
                                                        subpassDescription);
        return RenderPass(device, renderPassCreateInfo);
    }

    struct VertexAttributeInfo {
        uint32_t location;
        uint32_t binding;
        Format format;
        uint32_t offset;
    };

    inline Pipeline makeGraphicsPipeline(const Device &device, const PipelineCache &pipelineCache,
                                         const ShaderModule &vertexShaderModule,
                                         const ShaderModule &fragmentShaderModule, const uint32_t vertexStride,
                                         const std::vector<VertexAttributeInfo> &vertexAttributes,
                                         const PipelineLayout &pipelineLayout, const vk::RenderPass &renderPass,
                                         bool enableDepth, FrontFace frontFace = FrontFace::eCounterClockwise) {
        // Shader stages
        std::array shaderStages = {
                PipelineShaderStageCreateInfo{{}, ShaderStageFlagBits::eVertex, *vertexShaderModule, "main"},
                PipelineShaderStageCreateInfo{{}, ShaderStageFlagBits::eFragment, *fragmentShaderModule, "main"}};

        // Vertex input layout
        VertexInputBindingDescription bindingDesc{0, vertexStride, VertexInputRate::eVertex};

        std::vector<VertexInputAttributeDescription> attributeDescs;
        attributeDescs.reserve(vertexAttributes.size());
        for (const auto &[location, binding, format, offset]: vertexAttributes) {
            attributeDescs.emplace_back(location, binding, format, offset);
        }

        PipelineVertexInputStateCreateInfo vertexInputInfo{
                {}, 1, &bindingDesc, static_cast<uint32_t>(attributeDescs.size()), attributeDescs.data()};

        // Input assembly
        PipelineInputAssemblyStateCreateInfo inputAssembly{{}, PrimitiveTopology::eTriangleList, VK_FALSE};

        // Viewport & scissor (dynamic)
        PipelineViewportStateCreateInfo viewportState{{}, 1, nullptr, 1, nullptr};

        // Rasterization
        PipelineRasterizationStateCreateInfo rasterizer{
                {}, false, false, PolygonMode::eFill, CullModeFlagBits::eNone, frontFace, false, 0.f, 0.f, 0.f, 1.f};

        // Multisampling
        PipelineMultisampleStateCreateInfo multisampling{{}, SampleCountFlagBits::e1};

        // Depth stencil
        PipelineDepthStencilStateCreateInfo depthStencil{};
        if (enableDepth) {
            depthStencil = {{}, true, true, CompareOp::eLessOrEqual, false, false};
        }

        // Color blending
        PipelineColorBlendAttachmentState colorBlendAttachment{false,
                                                               BlendFactor::eZero,
                                                               BlendFactor::eZero,
                                                               BlendOp::eAdd,
                                                               BlendFactor::eZero,
                                                               BlendFactor::eZero,
                                                               BlendOp::eAdd,
                                                               ColorComponentFlagBits::eR | ColorComponentFlagBits::eG |
                                                                       ColorComponentFlagBits::eB |
                                                                       ColorComponentFlagBits::eA};

        PipelineColorBlendStateCreateInfo colorBlending{{}, false, LogicOp::eNoOp, 1, &colorBlendAttachment};

        // Dynamic states
        std::array dynamicStates = {DynamicState::eViewport, DynamicState::eScissor};
        PipelineDynamicStateCreateInfo dynamicState{{}, dynamicStates};

        // Combine into pipeline
        GraphicsPipelineCreateInfo pipelineInfo{{},
                                                static_cast<uint32_t>(shaderStages.size()),
                                                shaderStages.data(),
                                                &vertexInputInfo,
                                                &inputAssembly,
                                                nullptr,
                                                &viewportState,
                                                &rasterizer,
                                                &multisampling,
                                                enableDepth ? &depthStencil : nullptr,
                                                &colorBlending,
                                                &dynamicState,
                                                pipelineLayout,
                                                renderPass};

        return vk::raii::Pipeline(device, pipelineCache, pipelineInfo);
    }

    inline DescriptorSetLayout
    makeDescriptorSetLayout(Device const &device,
                            std::vector<std::tuple<DescriptorType, uint32_t, ShaderStageFlags>> const &bindingData,
                            const DescriptorSetLayoutCreateFlags flags = {}) {
        std::vector<DescriptorSetLayoutBinding> bindings(bindingData.size());
        for (size_t i = 0; i < bindingData.size(); i++) {
            bindings[i] = DescriptorSetLayoutBinding(static_cast<uint32_t>(i), std::get<0>(bindingData[i]),
                                                     std::get<1>(bindingData[i]), std::get<2>(bindingData[i]));
        }
        const DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo(flags, bindings);
        return DescriptorSetLayout(device, descriptorSetLayoutCreateInfo);
    }

    template<typename Func>
    void oneTimeSubmit(Device const &device, CommandPool const &commandPool, Queue const &queue, Func const &func) {
        CommandBuffer commandBuffer =
                std::move(CommandBuffers(device, {*commandPool, CommandBufferLevel::ePrimary, 1}).front());
        commandBuffer.begin(CommandBufferBeginInfo(CommandBufferUsageFlagBits::eOneTimeSubmit));
        func(commandBuffer);
        commandBuffer.end();
        const SubmitInfo submitInfo(nullptr, nullptr, *commandBuffer);
        queue.submit(submitInfo, nullptr);
        queue.waitIdle();
    }

    inline DeviceMemory allocateDeviceMemory(Device const &device, PhysicalDevice const &physicalDevice,
                                             MemoryRequirements const &memoryRequirements,
                                             const MemoryPropertyFlags memoryPropertyFlags) {
        const uint32_t memoryTypeIndex =
                vk::su::findMemoryType(physicalDevice, memoryRequirements.memoryTypeBits, memoryPropertyFlags);
        const MemoryAllocateInfo memoryAllocateInfo(memoryRequirements.size, memoryTypeIndex);
        return DeviceMemory(device, memoryAllocateInfo);
    }

    template<typename T>
    void copyToDevice(DeviceMemory const &deviceMemory, T const *pData, size_t count,
                      DeviceSize stride = sizeof(T)) {
        assert(sizeof(T) <= stride);
        auto deviceData = static_cast<uint8_t *>(deviceMemory.mapMemory(0, count * stride));
        if (stride == sizeof(T)) {
            memcpy(deviceData, pData, count * sizeof(T));
        } else {
            for (size_t i = 0; i < count; i++) {
                memcpy(deviceData, &pData[i], sizeof(T));
                deviceData += stride;
            }
        }
        deviceMemory.unmapMemory();
    }

    template<typename T>
    void copyToDevice(DeviceMemory const &deviceMemory, T const &data) {
        copyToDevice<T>(deviceMemory, &data, 1);
    }

    struct BufferData {
        BufferData(PhysicalDevice const &physicalDevice, Device const &device, const DeviceSize size,
                   const BufferUsageFlags usage,
                   MemoryPropertyFlags propertyFlags = MemoryPropertyFlagBits::eHostVisible |
                                                       MemoryPropertyFlagBits::eHostCoherent) :
            buffer(device, BufferCreateInfo({}, size, usage))
#if !defined(NDEBUG)
            ,
            m_size(size), m_usage(usage), m_propertyFlags(propertyFlags)
#endif
        {
            deviceMemory = allocateDeviceMemory(device, physicalDevice, buffer.getMemoryRequirements(), propertyFlags);
            buffer.bindMemory(deviceMemory, 0);
        }

        explicit BufferData(std::nullptr_t) : m_size{0} {}

        template<typename DataType>
        void upload(DataType const &data) const {
            assert((m_propertyFlags & vk::MemoryPropertyFlagBits::eHostCoherent) &&
                   (m_propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible));
            assert(sizeof(DataType) <= m_size);

            void *dataPtr = deviceMemory.mapMemory(0, sizeof(DataType));
            memcpy(dataPtr, &data, sizeof(DataType));
            deviceMemory.unmapMemory();
        }

        template<typename DataType>
        void upload(std::vector<DataType> const &data, const size_t stride = 0) const {
            assert(m_propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible);

            size_t elementSize = stride ? stride : sizeof(DataType);
            assert(sizeof(DataType) <= elementSize);

            copyToDevice(deviceMemory, data.data(), data.size(), elementSize);
        }

        template<typename DataType>
        void upload(PhysicalDevice const &physicalDevice, Device const &device, CommandPool const &commandPool,
                    Queue const &queue, std::vector<DataType> const &data, const size_t stride) const {
            assert(m_usage & vk::BufferUsageFlagBits::eTransferDst);
            assert(m_propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal);

            size_t elementSize = stride ? stride : sizeof(DataType);
            assert(sizeof(DataType) <= elementSize);

            const size_t dataSize = data.size() * elementSize;
            assert(dataSize <= m_size);

            BufferData stagingBuffer(physicalDevice, device, dataSize, BufferUsageFlagBits::eTransferSrc);
            copyToDevice(stagingBuffer.deviceMemory, data.data(), data.size(), elementSize);

            su::oneTimeSubmit(device, commandPool, queue, [&](CommandBuffer const &commandBuffer) {
                commandBuffer.copyBuffer(*stagingBuffer.buffer, *this->buffer, BufferCopy(0, 0, dataSize));
            });
        }

        // the DeviceMemory should be destroyed before the Buffer it is bound to; to get that order with the standard
        // destructor of the BufferData, the order of DeviceMemory and Buffer here matters
        DeviceMemory deviceMemory = nullptr;
        Buffer buffer = nullptr;
#if !defined(NDEBUG)
    private:
        DeviceSize m_size;
        BufferUsageFlags m_usage;
        MemoryPropertyFlags m_propertyFlags;
#endif
    };

    struct ImageData {
        ImageData(PhysicalDevice const &physicalDevice, Device const &device, const Format format_,
                  Extent2D const &extent, ImageTiling tiling, const ImageUsageFlags usage, ImageLayout initialLayout,
                  const MemoryPropertyFlags memoryProperties, ImageAspectFlags aspectMask) :
            format(format_), image(device, {ImageCreateFlags(),
                                            ImageType::e2D,
                                            format,
                                            Extent3D(extent, 1),
                                            1,
                                            1,
                                            SampleCountFlagBits::e1,
                                            tiling,
                                            usage | ImageUsageFlagBits::eSampled,
                                            SharingMode::eExclusive,
                                            {},
                                            initialLayout}) {
            deviceMemory =
                    allocateDeviceMemory(device, physicalDevice, image.getMemoryRequirements(), memoryProperties);
            image.bindMemory(deviceMemory, 0);
            imageView = ImageView(
                    device, ImageViewCreateInfo({}, image, ImageViewType::e2D, format, {}, {aspectMask, 0, 1, 0, 1}));
        }

        explicit ImageData(std::nullptr_t) : format{} {}

        // the DeviceMemory should be destroyed before the Image it is bound to; to get that order with the standard
        // destructor
        // of the ImageData, the order of DeviceMemory and Image here matters
        Format format;
        DeviceMemory deviceMemory = nullptr;
        Image image = nullptr;
        ImageView imageView = nullptr;
    };

    struct DepthBufferData : ImageData {
        DepthBufferData(PhysicalDevice const &physicalDevice, Device const &device, const Format format,
                        Extent2D const &extent) :
            ImageData(physicalDevice, device, format, extent, ImageTiling::eOptimal,
                      ImageUsageFlagBits::eDepthStencilAttachment, ImageLayout::eUndefined,
                      MemoryPropertyFlagBits::eDeviceLocal, ImageAspectFlagBits::eDepth) {}
    };
} // namespace vk::raii::su
