#include "Input.h"
#include <windows.h>
#include <iostream>

Input::Input(Application& app, InterfaceManager* interf) 
    : app(app), camera(app.getCamera()), interf(interf),
      mouseCaptured(false),
      rightMouseDown(false),
      lastX(0), lastY(0),
      firstMouse(true),
      mouseSensitivityScale(1.0f),
      debugEnabled(true)
{
    for (int i = 0; i < 6; i++) keys[i] = false;
    
    if (debugEnabled) {
        std::cout << "[INPUT] Initialized" << std::endl;
    }
}

void Input::SetMouseCapture(bool capture) {
    if (mouseCaptured == capture) return;
    
    mouseCaptured = capture;
    HWND hwnd = interf->getHWND();
    
    if (capture) {
        ShowCursor(FALSE);
        SetCapture(hwnd);
        
        RECT rect;
        GetClientRect(hwnd, &rect);
        POINT center;
        center.x = (rect.left + rect.right) / 2;
        center.y = (rect.top + rect.bottom) / 2;
        ClientToScreen(hwnd, &center);
        SetCursorPos(center.x, center.y);
        
        lastX = (float)center.x;
        lastY = (float)center.y;
        firstMouse = true;
        
        if (debugEnabled) {
            std::cout << "[INPUT] Mouse captured" << std::endl;
        }
    } else {
        ShowCursor(TRUE);
        ReleaseCapture();
        
        if (debugEnabled) {
            std::cout << "[INPUT] Mouse released" << std::endl;
        }
    }
}

void Input::UpdateMouseCapture(HWND hwnd) {
    if (GetAsyncKeyState(VK_RBUTTON) & 0x8000) {
        if (!rightMouseDown) {
            rightMouseDown = true;
            SetMouseCapture(!mouseCaptured);
        }
    } else {
        rightMouseDown = false;
    }
}

void Input::UpdateKeyboardMovement(float deltaTime, HWND hwnd) {
    if (!mouseCaptured) return;
    
    bool w = (GetAsyncKeyState('W') & 0x8000) != 0;
    bool s = (GetAsyncKeyState('S') & 0x8000) != 0;
    bool a = (GetAsyncKeyState('A') & 0x8000) != 0;
    bool d = (GetAsyncKeyState('D') & 0x8000) != 0;
    bool space = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
    bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
    bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    
    static bool lastW=false, lastS=false, lastA=false, lastD=false, lastSpace=false, lastCtrl=false, lastShift=false;
    
    if (debugEnabled) {
        if (w != lastW) std::cout << "[INPUT] W=" << w << std::endl;
        if (s != lastS) std::cout << "[INPUT] S=" << s << std::endl;
        if (a != lastA) std::cout << "[INPUT] A=" << a << std::endl;
        if (d != lastD) std::cout << "[INPUT] D=" << d << std::endl;
        if (space != lastSpace && space) std::cout << "[INPUT] SPACE (UP)" << std::endl;
        if (ctrl != lastCtrl && ctrl) std::cout << "[INPUT] CTRL (DOWN)" << std::endl;
        if (shift != lastShift && shift) std::cout << "[INPUT] SHIFT (SPRINT x" << camera.GetMovementSpeed() / 8.0f << ")" << std::endl;
    }
    
    lastW=w; lastS=s; lastA=a; lastD=d; lastSpace=space; lastCtrl=ctrl; lastShift=shift;
    
    camera.SetSprinting(shift);
    
    if (w) camera.ProcessKeyboard(FORWARD, deltaTime);
    if (s) camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (a) camera.ProcessKeyboard(LEFT, deltaTime);
    if (d) camera.ProcessKeyboard(RIGHT, deltaTime);
    if (space) camera.ProcessKeyboard(UP, deltaTime);
    if (ctrl) camera.ProcessKeyboard(DOWN, deltaTime);
    
    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
        static bool escPressed = false;
        if (!escPressed) {
            escPressed = true;
            if (mouseCaptured) {
                if (debugEnabled) std::cout << "[INPUT] ESC: releasing mouse" << std::endl;
                SetMouseCapture(false);
            } else {
                if (debugEnabled) std::cout << "[INPUT] ESC: exiting" << std::endl;
                PostMessage(hwnd, WM_CLOSE, 0, 0);
            }
        }
    } else {
        static bool escPressed = false;
        escPressed = false;
    }
    
    if (GetAsyncKeyState('R') & 0x8000) {
        static bool rPressed = false;
        if (!rPressed) {
            rPressed = true;
            if (debugEnabled) std::cout << "[INPUT] R: resetting camera" << std::endl;
            camera.ResetCamera();
        }
    } else {
        static bool rPressed = false;
        rPressed = false;
    }
}

void Input::processInputWin32(float deltaTime, HWND hwnd) {
    UpdateMouseCapture(hwnd);
    UpdateKeyboardMovement(deltaTime, hwnd);
}

void Input::processMouseWin32(float xpos, float ypos) {
    if (!mouseCaptured) {
        firstMouse = true;
        return;
    }
    
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    
    float sensitivity = camera.GetMouseSensitivity() * mouseSensitivityScale;
    xoffset *= sensitivity;
    yoffset *= sensitivity;
    
    camera.ProcessMouseMovement(xoffset, yoffset, true);
    
    lastX = xpos;
    lastY = ypos;
}

void Input::Update(float deltaTime) {
    camera.UpdateSmoothRotation(deltaTime);
}

void Input::processInput(float deltaTime) {
    // Заглушка для совместимости
}