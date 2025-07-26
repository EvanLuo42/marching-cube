#include "ShaderManager.h"
#include <filesystem>

static void printSlangDiagnostics(const Slang::ComPtr<slang::IBlob>& diagnosticsBlob) {
    if (diagnosticsBlob) {
        const char* msg = static_cast<const char*>(diagnosticsBlob->getBufferPointer());
        std::printf("[Slang Error] %s\n", msg);
    }
}

void ShaderManager::compile() {
    std::vector<std::string> files{};
    for (const auto &entry : std::filesystem::directory_iterator("../Shaders")) {
        if (entry.is_regular_file()) {
            files.emplace_back(entry.path().stem());
        }
    }

    Slang::ComPtr<slang::IGlobalSession> globalSession;
    slang::createGlobalSession(globalSession.writeRef());

    slang::TargetDesc targetDesc = {};
    targetDesc.format = SLANG_SPIRV;
    targetDesc.profile = globalSession->findProfile("spirv_1_3");
    targetDesc.flags = 0;

    std::vector<slang::TargetDesc> targets{};

    targets.reserve(files.size());
    for (auto i = 0; i < files.size(); i++) {
        targets.emplace_back(targetDesc);
    }

    slang::SessionDesc sessionDesc = {};
    sessionDesc.targets = targets.data();
    sessionDesc.targetCount = static_cast<int>(files.size());
    sessionDesc.compilerOptionEntryCount = 0;

    const char* searchPaths[] = { "../Shaders/" };
    sessionDesc.searchPaths = searchPaths;
    sessionDesc.searchPathCount = 1;

    Slang::ComPtr<slang::ISession> session;
    globalSession->createSession(sessionDesc, session.writeRef());

    for (const auto& file: files) {
        slang::IModule* slangModule = nullptr;
        {
            Slang::ComPtr<slang::IBlob> diagnosticBlob;
            slangModule = session->loadModule(file.c_str(), diagnosticBlob.writeRef());
            if (!slangModule) {
                printSlangDiagnostics(diagnosticBlob);
                exit(-1);
            }
        }

        Slang::ComPtr<slang::IEntryPoint> vertexEntry;
        slangModule->findEntryPointByName("vertexMain", vertexEntry.writeRef());

        Slang::ComPtr<slang::IEntryPoint> fragmentEntry;
        slangModule->findEntryPointByName("fragmentMain", fragmentEntry.writeRef());

        std::vector<slang::IComponentType*> componentTypes = { slangModule, vertexEntry, fragmentEntry };

        Slang::ComPtr<slang::IComponentType> composedProgram;
        {
            Slang::ComPtr<slang::IBlob> diagnosticsBlob;
            const SlangResult result = session->createCompositeComponentType(
                componentTypes.data(),
                static_cast<int>(componentTypes.size()),
                composedProgram.writeRef(),
                diagnosticsBlob.writeRef());
            if (SLANG_FAILED(result)) {
                printSlangDiagnostics(diagnosticsBlob);
                exit(-1);
            }
        }

        Slang::ComPtr<slang::IBlob> vertSpirvCode;
        {
            Slang::ComPtr<slang::IBlob> diagnosticsBlob;
            const SlangResult result = composedProgram->getEntryPointCode(
                0,
                0,
                vertSpirvCode.writeRef(),
                diagnosticsBlob.writeRef());
            if (SLANG_FAILED(result)) {
                printSlangDiagnostics(diagnosticsBlob);
                exit(-1);
            }
        }

        Slang::ComPtr<slang::IBlob> fragSpirvCode;
        {
            Slang::ComPtr<slang::IBlob> diagnosticsBlob;
            const SlangResult result = composedProgram->getEntryPointCode(
                1,
                0,
                fragSpirvCode.writeRef(),
                diagnosticsBlob.writeRef());
            if (SLANG_FAILED(result)) {
                printSlangDiagnostics(diagnosticsBlob);
                exit(-1);
            }
        }

        spirvCodes[file] = {vertSpirvCode, fragSpirvCode};
    }
}
