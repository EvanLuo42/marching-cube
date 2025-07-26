#include "Resource/ShaderManager.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Render/RenderSettings.h"
#include "Render/Renderer.h"
#include "Terrain/TerrainEditor.h"

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow *window = glfwCreateWindow(800, 600, "Marching Cube Terrain", nullptr, nullptr);
    Renderer renderer{window};
    TerrainEditor terrainEditor{};

    RenderSettings renderSettings{};

    float deltaTime = 0;
    float lastFrame = 0;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        const auto currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        renderer.cameraUpdate(deltaTime);

        terrainEditor.update(deltaTime);

        if (terrainEditor.isEdited()) {
            auto [vertices, indices] = terrainEditor.getMesh();
            renderer.updateBuffers(vertices, indices);
        }

        renderer.beginFrame();

        terrainEditor.renderUI();

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


