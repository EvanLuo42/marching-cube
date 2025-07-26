#include "Camera.h"

#include "GLFW/glfw3.h"
#include "glm/ext/matrix_transform.hpp"
#include "imgui.h"

Camera::Camera(GLFWwindow* win) : window(win) {
    updateCameraVectors();
}

void Camera::update(const float deltaTime) {
    processKeyboard(deltaTime);
    processMouse();
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(position, position + front, up);
}

void Camera::processKeyboard(const float dt) {
    if (ImGui::GetIO().WantCaptureKeyboard) return;

    const float velocity = speed * dt;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        position += front * velocity;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        position -= front * velocity;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        position -= right * velocity;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        position += right * velocity;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        position += worldUp * velocity;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        position -= worldUp * velocity;
}

void Camera::processMouse() {
    static bool rightMousePressedLastFrame = false;

    if (const bool rightMousePressedNow = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT); !rightMousePressedNow) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        rightMousePressedLastFrame = false;
        return;
    }

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    int width, height;
    glfwGetWindowSize(window, &width, &height);
    const double centerX = width / 2.0;
    const double centerY = height / 2.0;

    if (!rightMousePressedLastFrame) {
        glfwSetCursorPos(window, centerX, centerY);
        lastX = static_cast<float>(centerX);
        lastY = static_cast<float>(centerY);
        rightMousePressedLastFrame = true;
        return;
    }

    double xPos, yPos;
    glfwGetCursorPos(window, &xPos, &yPos);

    auto xOffset = static_cast<float>(xPos - lastX);
    auto yOffset = static_cast<float>(lastY - yPos);

    lastX = static_cast<float>(xPos);
    lastY = static_cast<float>(yPos);

    xOffset *= sensitivity;
    yOffset *= sensitivity;

    yaw   += xOffset;
    pitch += yOffset;

    pitch = glm::clamp(pitch, -89.0f, 89.0f);

    updateCameraVectors();

    glfwSetCursorPos(window, centerX, centerY);
    lastX = static_cast<float>(centerX);
    lastY = static_cast<float>(centerY);
}

void Camera::updateCameraVectors() {
    glm::vec3 newFront;
    newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    newFront.y = sin(glm::radians(pitch));
    newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(newFront);

    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}
