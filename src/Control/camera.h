#ifndef CAMERA_H
#define CAMERA_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

class Camera {
public:
    Camera(glm::vec3 position = glm::vec3(0.0f, 50.0f, 150.0f),  // ИЗМЕНЕНО
           glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
           float yaw = -90.0f, float pitch = 0.0f);
    
    glm::mat4 GetViewMatrix() const;
    
    void ProcessKeyboard(int direction, float deltaTime);
    void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);
    void ProcessMouseScroll(float yoffset);
    
    void UpdateSmoothRotation(float deltaTime);
    void SetTargetRotation(float yaw, float pitch);
    void SetSmoothing(float yawSpeed, float pitchSpeed);
    void SetSpeedMultiplier(float multiplier);
    void ResetCamera();
    
    // Геттеры
    glm::vec3 GetPosition() const { return Position; }
    glm::vec3 GetFront() const { return Front; }
    glm::vec3 GetUp() const { return Up; }
    glm::vec3 GetRight() const { return Right; }
    float GetZoom() const { return Zoom; }
    float GetMovementSpeed() const { return MovementSpeed; }
    float GetMouseSensitivity() const { return MouseSensitivity; }
    bool IsSprinting() const { return isSprinting; }
    
    // Сеттеры
    void SetPosition(const glm::vec3& position) { Position = position; }
    void SetMovementSpeed(float speed) { MovementSpeed = speed; }
    void SetMouseSensitivity(float sensitivity) { MouseSensitivity = sensitivity; }
    void SetSprinting(bool sprint) { isSprinting = sprint; }
    
private:
    void updateCameraVectors();
    
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;
    
    float Yaw;
    float Pitch;
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;
    
    float TargetYaw;
    float TargetPitch;
    float CurrentYaw;
    float CurrentPitch;
    float YawSmoothing;
    float PitchSmoothing;
    
    float BaseSpeed;
    float SprintMultiplier;
    bool isSprinting;
    
    bool debugEnabled;
};

#endif