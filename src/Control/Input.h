#ifndef INPUT_H
#define INPUT_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include "camera.h"
#include "../Application/application.h"
#include "../Interface/InterfaceManager.h"

class Input {
public:
    Input(Application& app, InterfaceManager* interf);
    
    void processInput(float deltaTime);
    void processInputWin32(float deltaTime, HWND hwnd);
    void processMouseWin32(float xpos, float ypos);
    void SetMouseCapture(bool capture);
    bool IsMouseCaptured() const;
    void Update(float deltaTime);
    void EnableDebug(bool enable);
    
private:
    void UpdateKeyboardMovement(float deltaTime, HWND hwnd);
    void UpdateMouseCapture(HWND hwnd);
    
    Application& app;
    Camera& camera;
    InterfaceManager* interf;
    
    bool mouseCaptured;
    bool rightMouseDown;
    float lastX, lastY;
    bool firstMouse;
    bool keys[6];
    float mouseSensitivityScale;
    bool debugEnabled;
};

#endif