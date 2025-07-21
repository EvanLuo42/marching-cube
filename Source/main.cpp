#include "Render/RenderContext.h"
#include "Resource/ShaderManager.h"

int main() {
    ShaderManager shaderManager{};
    shaderManager.compile();
    std::printf("%p", shaderManager.getSpirvCode("Triangle").value()->getBufferPointer());
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Marching Cube Terrain", nullptr, nullptr);
    RenderContext renderContext{window, "Marching Cube Terrain"};
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }
    return 0;
}