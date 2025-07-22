#include "Render/Renderer.h"
#include "Resource/ShaderManager.h"

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Marching Cube Terrain", nullptr, nullptr);
    Renderer renderer{window};
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }
    return 0;
}