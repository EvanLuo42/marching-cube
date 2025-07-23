#pragma once
#include "../Resource/ShaderManager.h"
#include "RenderContext.h"


class Renderer {
    RenderContext renderContext;
    ShaderManager shaderManager;

public:
    explicit Renderer(GLFWwindow *window) : renderContext{window} {
        shaderManager = {};
        shaderManager.compile();
    }
};