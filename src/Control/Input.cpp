#include "Input.h"
#include "../Interface/BufferLayer.h"
#include "../Core/SecondComplexity/Scene/SceneManager.h"
#include <windows.h>
#include <iostream>

Input::Input(Application& app, InterfaceManager* interf) 
    : app(app), camera(app.getCamera()), interf(interf),
      mouseCaptured(false), rightMouseDown(false), lastX(0), lastY(0),
      firstMouse(true), mouseSensitivityScale(1.0f), debugEnabled(true) {
    for (int i = 0; i < 6; i++) keys[i] = false;
}

void Input::SetMouseCapture(bool capture) {
    if (BufferLayer::Instance().showContextMenu || 
        BufferLayer::Instance().isRenaming || 
        BufferLayer::Instance().isCreating) {
        return;
    }
    
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
    } else {
        ShowCursor(TRUE);
        ReleaseCapture();
    }
}

void Input::UpdateMouseCapture(HWND hwnd) {
    if (GetAsyncKeyState(VK_RBUTTON) & 0x8000) {
        if (!rightMouseDown) {
            rightMouseDown = true;
            
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(hwnd, &pt);
            
            RECT clientRect;
            GetClientRect(hwnd, &clientRect);
            
            Panel* view3D = interf->getPanelManager()->get3D();
            if (view3D && view3D->visible) {
                int viewY = clientRect.bottom - (view3D->getY() + view3D->getH());
                if (pt.x >= view3D->getX() && pt.x <= view3D->getX() + view3D->getW() &&
                    pt.y >= viewY && pt.y <= viewY + view3D->getH()) {
                    SetMouseCapture(!mouseCaptured);
                }
            }
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
    
    camera.SetSprinting(shift);
    
    if (w) camera.ProcessKeyboard(FORWARD, deltaTime);
    if (s) camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (a) camera.ProcessKeyboard(LEFT, deltaTime);
    if (d) camera.ProcessKeyboard(RIGHT, deltaTime);
    if (space) camera.ProcessKeyboard(UP, deltaTime);
    if (ctrl) camera.ProcessKeyboard(DOWN, deltaTime);
    
    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
        if (mouseCaptured) SetMouseCapture(false);
    }
    
    static bool rPressed = false;
    if (GetAsyncKeyState('R') & 0x8000) {
        if (!rPressed) {
            rPressed = true;
            auto& sm = SceneManager::Instance();
            sm.ResetCamera();
            camera.ResetCamera();
        }
    } else { 
        rPressed = false; 
    }
}

void Input::processInputWin32(float deltaTime, HWND hwnd) {
    UpdateMouseCapture(hwnd);
    UpdateKeyboardMovement(deltaTime, hwnd);
}

void Input::processMouseWin32(float xpos, float ypos) {
    if (!mouseCaptured) { firstMouse = true; return; }
    
    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
    
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    xoffset *= camera.GetMouseSensitivity() * mouseSensitivityScale;
    yoffset *= camera.GetMouseSensitivity() * mouseSensitivityScale;
    
    camera.ProcessMouseMovement(xoffset, yoffset, true);
    
    auto& sm = SceneManager::Instance();
    sm.RotateCamera(xoffset, yoffset);
    
    lastX = xpos;
    lastY = ypos;
}

void Input::processMouseWheel(int delta, HWND hwnd) {
    POINT mousePos;
    GetCursorPos(&mousePos);
    ScreenToClient(hwnd, &mousePos);
    
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    
    Panel* view3D = interf->getPanelManager()->get3D();
    if (view3D && view3D->visible) {
        int viewY = clientRect.bottom - (view3D->getY() + view3D->getH());
        bool isInViewport = (mousePos.x >= view3D->getX() && mousePos.x <= view3D->getX() + view3D->getW() &&
                             mousePos.y >= viewY && mousePos.y <= viewY + view3D->getH());
        
        if (isInViewport) {
            auto& sm = SceneManager::Instance();
            float zoomDelta = (float)delta / 120.0f;
            sm.ZoomCamera(zoomDelta);
        }
    }
}

void Input::Update(float deltaTime) { 
    camera.UpdateSmoothRotation(deltaTime); 
}

void Input::processInput(float deltaTime) {}

void Input::EnableDebug(bool enable) { 
    debugEnabled = enable; 
}

bool Input::IsMouseCaptured() const { 
    return mouseCaptured; 
}