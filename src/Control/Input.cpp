#include "Input.h"
#include <windows.h>

Input::Input(Application& app) : app(app), camera(app.getCamera()) {
}

void Input::processInput(float deltaTime) {
    // Заглушка: для Win32 используем processInputWin32
}

// WIN32 ВЕРСИЯ
void Input::processInputWin32(float deltaTime, HWND hwnd) {
    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
        PostMessage(hwnd, WM_CLOSE, 0, 0);
    }
    
    if (GetAsyncKeyState('W') & 0x8000)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (GetAsyncKeyState('S') & 0x8000)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (GetAsyncKeyState('A') & 0x8000)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (GetAsyncKeyState('D') & 0x8000)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if (GetAsyncKeyState(VK_SPACE) & 0x8000)
        camera.ProcessKeyboard(UP, deltaTime);
    if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
        camera.ProcessKeyboard(DOWN, deltaTime);
}

void Input::processMouseWin32(float xpos, float ypos) {
    static bool firstMouse = true;
    static float lastX = 400.0f;
    static float lastY = 300.0f;
    
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    
    lastX = xpos;
    lastY = ypos;
    
    camera.ProcessMouseMovement(xoffset, yoffset);
}