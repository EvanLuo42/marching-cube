#pragma once
#include <slang/slang-com-ptr.h>
#include <unordered_map>
#include <string>

class ShaderManager {
    std::unordered_map<std::string, Slang::ComPtr<slang::IBlob>> spirvCodes;

public:
    void compile();
};
