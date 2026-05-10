#include "camera.h"
#include <iostream>

Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
    : Front(glm::vec3(0.0f, 0.0f, -1.0f)),
      MovementSpeed(15.0f),
      MouseSensitivity(0.15f),
      Zoom(45.0f),
      BaseSpeed(15.0f),
      SprintMultiplier(2.0f),
      isSprinting(false)
{
    Position = position;
    WorldUp = up;
    Yaw = yaw;
    Pitch = pitch;
    updateCameraVectors();
}

glm::mat4 Camera::GetViewMatrix() const {
    return glm::lookAt(Position, Position + Front, Up);
}

void Camera::ProcessKeyboard(int direction, float deltaTime) {
    float speed = MovementSpeed * deltaTime;
    if (isSprinting) speed *= SprintMultiplier;
    
    switch(direction) {
        case FORWARD:   Position += Front * speed; break;
        case BACKWARD:  Position -= Front * speed; break;
        case LEFT:      Position -= Right * speed; break;
        case RIGHT:     Position += Right * speed; break;
        case UP:        Position += Up * speed; break;
        case DOWN:      Position -= Up * speed; break;
    }
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset) {
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;
    
    Yaw += xoffset;
    Pitch += yoffset;
    
    if (Pitch > 89.0f) Pitch = 89.0f;
    if (Pitch < -89.0f) Pitch = -89.0f;
    
    updateCameraVectors();
}

void Camera::ProcessMouseScroll(float yoffset) {
    Zoom -= yoffset;
    if (Zoom < 1.0f) Zoom = 1.0f;
    if (Zoom > 90.0f) Zoom = 90.0f;
}

void Camera::ResetCamera() {
    Position = glm::vec3(0.0f, 5.0f, 15.0f);
    Yaw = -90.0f;
    Pitch = 0.0f;
    Zoom = 45.0f;
    isSprinting = false;
    updateCameraVectors();
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