#ifndef INPUT_H
#define INPUT_H

#include "../Application/application.h"
#include "../Interface/InterfaceManager.h"
#include <windows.h>

class Input {
public:
    Input(Application& app, InterfaceManager* interf);
    ~Input() = default;
    
    void processInputWin32(float deltaTime, HWND hwnd);
    void processMouseWin32(float xpos, float ypos);
    void processMouseWheel(int delta, HWND hwnd);
    void Update(float deltaTime);
    void processInput(float deltaTime);
    void EnableDebug(bool enable);
    bool IsMouseCaptured() const;
    void SetMouseCapture(bool capture);
    
private:
    void UpdateMouseCapture(HWND hwnd);
    void UpdateKeyboardMovement(float deltaTime, HWND hwnd);
    
    Application& app;
    Camera& camera;
    InterfaceManager* interf;
    
    bool mouseCaptured;
    bool rightMouseDown;
    float lastX, lastY;
    bool firstMouse;
    float mouseSensitivityScale;
    bool debugEnabled;
    bool keys[6];
};

#endif