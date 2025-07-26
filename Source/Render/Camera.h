#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

class Camera {
public:
    GLFWwindow* window;

    glm::vec3 position = {0.0f, 0.0f, 3.0f};
    glm::vec3 front = {0.0f, 0.0f, -1.0f};
    glm::vec3 up = {0.0f, 1.0f, 0.0f};
    glm::vec3 right = {1.0f, 0.0f, 0.0f};
    glm::vec3 worldUp = {0.0f, 1.0f, 0.0f};

    float yaw = -90.0f;
    float pitch = 0.0f;
    float speed = 5.0f;
    float sensitivity = 0.1f;
    float lastX = 400, lastY = 300;
    bool firstMouse = true;

    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    explicit Camera(GLFWwindow* win);

    void update();
    [[nodiscard]] glm::mat4 getViewMatrix() const;

private:
    void updateCameraVectors();
    void processKeyboard(float dt);
    void processMouse();
};