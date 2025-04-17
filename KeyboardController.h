#pragma once
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

class KeyboardController {
public:
    KeyboardController();

    void processInput(GLFWwindow* window, float deltaTime);
    void processMouseMovement(double xpos, double ypos);

    glm::vec3 getCameraPosition() const;
    glm::vec3 getCameraDirection() const;

private:
    glm::vec3 position;
    glm::vec3 direction;
    float speed;

    // Mouse control
    float yaw, pitch;
    bool firstMouse;
    float lastX, lastY;
    void updateCameraVectors();
};
