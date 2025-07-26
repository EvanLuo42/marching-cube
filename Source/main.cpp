#include "Resource/ShaderManager.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Render/Renderer.h"

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow *window = glfwCreateWindow(800, 600, "Marching Cube Terrain", nullptr, nullptr);
    Renderer renderer{window};

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        renderer.beginFrame();

        ImGui::ShowDemoWindow();

        renderer.renderScene();
        renderer.renderUI();
        renderer.endFrame();
    }
    return 0;
}


