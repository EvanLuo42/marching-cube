#include "Resource/ShaderManager.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Render/RenderSettings.h"
#include "Render/Renderer.h"

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow *window = glfwCreateWindow(800, 600, "Marching Cube Terrain", nullptr, nullptr);
    Renderer renderer{window};

    RenderSettings renderSettings{};

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        renderer.cameraUpdate();

        renderer.beginFrame();

        ImGui::Begin("Lighting Debug");
        ImGui::InputFloat3("Light Position", &renderSettings.lighting.lightPos.x);
        ImGui::SliderFloat("Shininess", &renderSettings.lighting.shininess, 1.0f, 128.0f);
        ImGui::End();

        renderer.renderScene(renderSettings);
        renderer.renderUI();
        renderer.endFrame();
    }
    return 0;
}


