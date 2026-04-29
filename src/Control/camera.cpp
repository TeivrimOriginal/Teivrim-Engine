#include "camera.h"

Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
    : Front(glm::vec3(0.0f, 0.0f, -1.0f)), 
      MovementSpeed(2.0f),  // Увеличил скорость
      MouseSensitivity(1.10f), 
      Zoom(45.0f),
      TargetYaw(yaw),
      TargetPitch(pitch),
      CurrentYaw(yaw),
      CurrentPitch(pitch),
      YawSmoothing(12.0f),
      PitchSmoothing(12.0f),
      BaseSpeed(15.0f),
      SprintMultiplier(2.5f),
      isSprinting(false),
      debugEnabled(false)  // Отключаем отладку для чистоты
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
        case FORWARD: Position += Front * speed; break;
        case BACKWARD: Position -= Front * speed; break;
        case LEFT: Position -= Right * speed; break;
        case RIGHT: Position += Right * speed; break;
        case UP: Position += Up * speed; break;
        case DOWN: Position -= Up * speed; break;
    }
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch) {
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;
    SetTargetRotation(Yaw + xoffset, Pitch + yoffset);
}

void Camera::ProcessMouseScroll(float yoffset) {
    Zoom -= yoffset;
    if (Zoom < 1.0f) Zoom = 1.0f;
    if (Zoom > 90.0f) Zoom = 90.0f;
}

void Camera::SetTargetRotation(float yaw, float pitch) {
    TargetYaw = yaw;
    TargetPitch = pitch;
    if (TargetPitch > 89.0f) TargetPitch = 89.0f;
    if (TargetPitch < -89.0f) TargetPitch = -89.0f;
}

void Camera::UpdateSmoothRotation(float deltaTime) {
    float yawDiff = TargetYaw - CurrentYaw;
    while (yawDiff > 180.0f) yawDiff -= 360.0f;
    while (yawDiff < -180.0f) yawDiff += 360.0f;
    float pitchDiff = TargetPitch - CurrentPitch;
    
    float yawStep = yawDiff * std::min(1.0f, YawSmoothing * deltaTime);
    float pitchStep = pitchDiff * std::min(1.0f, PitchSmoothing * deltaTime);
    
    CurrentYaw += yawStep;
    CurrentPitch += pitchStep;
    
    Yaw = CurrentYaw;
    Pitch = CurrentPitch;
    updateCameraVectors();
}

void Camera::SetSmoothing(float yawSpeed, float pitchSpeed) {
    YawSmoothing = yawSpeed;
    PitchSmoothing = pitchSpeed;
}

void Camera::SetSpeedMultiplier(float multiplier) {
    MovementSpeed = BaseSpeed * multiplier;
}

void Camera::ResetCamera() {
    // МОДЕЛЬ В ЦЕНТРЕ (0,0,0), КАМЕРА СМОТРИТ НА НЕЁ
    Position = glm::vec3(0.0f, 50.0f, 150.0f);  // Спереди модели
    TargetYaw = -90.0f;   // Смотрим в положительную Z
    TargetPitch = 0.0f;
    CurrentYaw = -90.0f;
    CurrentPitch = 0.0f;
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