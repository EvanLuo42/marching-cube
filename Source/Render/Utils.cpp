#include "Utils.h"

vk::raii::Instance vk::raii::su::makeInstance(const Context &context, const std::string &appName,
                                              const std::string &engineName, const std::vector<std::string> &layers,
                                              const std::vector<std::string> &extensions, const uint32_t apiVersion) {
    const ApplicationInfo applicationInfo{appName.c_str(), 1, engineName.c_str(), 1, apiVersion};
    const std::vector enabledLayers = vk::su::gatherLayers(layers,
#if !defined(NDEBUG)
                                                           context.enumerateInstanceLayerProperties()
#endif
    );
    const std::vector enabledExtensions = vk::su::gatherExtensions(extensions,
#if !defined(NDEBUG)
                                                                   context.enumerateInstanceExtensionProperties()
#endif
    );
#if defined(__APPLE__)
    constexpr auto flags = InstanceCreateFlagBits::eEnumeratePortabilityKHR;
#endif
#if defined(NDEBUG)
    StructureChain<InstanceCreateInfo>
#else
    StructureChain<InstanceCreateInfo, DebugUtilsMessengerCreateInfoEXT>
#endif
            instanceCreateInfoChain =
                    vk::su::makeInstanceCreateInfoChain(flags, applicationInfo, enabledLayers, enabledExtensions);
    return Instance{context, instanceCreateInfoChain.get<InstanceCreateInfo>()};
}

vk::PresentModeKHR vk::su::pickPresentMode(std::vector<PresentModeKHR> const &presentModes) {
    auto pickedMode = PresentModeKHR::eFifo;
    for (const auto &presentMode: presentModes) {
        if (presentMode == PresentModeKHR::eMailbox) {
            pickedMode = presentMode;
            break;
        }

        if (presentMode == PresentModeKHR::eImmediate) {
            pickedMode = presentMode;
        }
    }
    return pickedMode;
}
