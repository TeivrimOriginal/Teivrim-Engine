#include "camera.h"
#include <iostream>

Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch) 
    : Front(glm::vec3(0.0f, 0.0f, -1.0f)), 
      MovementSpeed(5.0f), 
      MouseSensitivity(0.1f), 
      Zoom(45.0f) {
    Position = position;
    WorldUp = up;
    Yaw = yaw;
    Pitch = pitch;
    updateCameraVectors();
}

glm::mat4 Camera::GetViewMatrix() {
    return glm::lookAt(Position, Position + Front, Up);
}

void Camera::ProcessKeyboard(int direction, float deltaTime) {
    float velocity = MovementSpeed * deltaTime;
    
    switch (direction) {
        case FORWARD:
            Position += Front * velocity;
            std::cout << "Moving FORWARD - Position: (" 
                      << Position.x << ", " << Position.y << ", " << Position.z << ")" << std::endl;
            break;
        case BACKWARD:
            Position -= Front * velocity;
            std::cout << "Moving BACKWARD - Position: (" 
                      << Position.x << ", " << Position.y << ", " << Position.z << ")" << std::endl;
            break;
        case LEFT:
            Position -= Right * velocity;
            std::cout << "Moving LEFT - Position: (" 
                      << Position.x << ", " << Position.y << ", " << Position.z << ")" << std::endl;
            break;
        case RIGHT:
            Position += Right * velocity;
            std::cout << "Moving RIGHT - Position: (" 
                      << Position.x << ", " << Position.y << ", " << Position.z << ")" << std::endl;
            break;
        case UP:
            Position += WorldUp * velocity;
            std::cout << "Moving UP - Position: (" 
                      << Position.x << ", " << Position.y << ", " << Position.z << ")" << std::endl;
            break;
        case DOWN:
            Position -= WorldUp * velocity;
            std::cout << "Moving DOWN - Position: (" 
                      << Position.x << ", " << Position.y << ", " << Position.z << ")" << std::endl;
            break;
    }
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch) {
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;

    Yaw += xoffset;
    Pitch += yoffset;

    if (constrainPitch) {
        if (Pitch > 89.0f)
            Pitch = 89.0f;
        if (Pitch < -89.0f)
            Pitch = -89.0f;
    }

    updateCameraVectors();
}

void Camera::ProcessMouseScroll(float yoffset) {
    Zoom -= (float)yoffset;
    if (Zoom < 1.0f)
        Zoom = 1.0f;
    if (Zoom > 90.0f)
        Zoom = 90.0f;
}

void Camera::updateCameraVectors() {
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    Front = glm::normalize(front);
    
    Right = glm::normalize(glm::cross(Front, WorldUp));
    Up = glm::normalize(glm::cross(Right, Front));
}