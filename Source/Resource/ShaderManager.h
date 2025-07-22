#pragma once
#include <slang/slang-com-ptr.h>
#include <unordered_map>
#include <string>

class ShaderManager {
    using SpirvCode = Slang::ComPtr<slang::IBlob>;
    std::unordered_map<std::string, std::tuple<SpirvCode, SpirvCode>> spirvCodes;

public:
    void compile();
    [[nodiscard]] std::optional<std::tuple<SpirvCode, SpirvCode>> getSpirvCode(const std::string& name) const {
        if (const auto it = spirvCodes.find(name); it != spirvCodes.end()) {
            return it->second;
        }
        return std::nullopt;
    }
};
