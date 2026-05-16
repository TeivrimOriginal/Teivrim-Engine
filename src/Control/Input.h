// Input.h
#ifndef INPUT_H
#define INPUT_H

#include "../Application/application.h"
#include <windows.h>

class InterfaceManager;

class Input {
public:
    Input(Application& app, InterfaceManager* uiManager);
    ~Input();
    
    void processMouseWin32(float x, float y);
    void processInputWin32(float deltaTime, HWND hwnd);
    void Update(float deltaTime);
    void processMouseWheel(int delta, HWND hwnd);
    
private:
    void UpdateKeyboardMovement(float deltaTime, HWND hwnd);
    void UpdateMouseCapture(HWND hwnd);
    
    Application& m_app;
    InterfaceManager* m_uiManager;
    bool m_captured;
    float m_lastX, m_lastY;
};

#endif