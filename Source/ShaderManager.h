#pragma once
#include <slang/slang-com-ptr.h>
#include <unordered_map>
#include <string>

class ShaderManager {
    std::unordered_map<std::string, Slang::ComPtr<slang::IBlob>> spirvCodes;

public:
    void compile();
    [[nodiscard]] std::optional<Slang::ComPtr<slang::IBlob>> getSpirvCode(const std::string& name) const {
        if (const auto it = spirvCodes.find(name); it != spirvCodes.end()) {
            return it->second;
        }
        return std::nullopt;
    }
};
