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
    std::cout << "[Input] Initialized" << std::endl;
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
        std::cout << "[Input] Mouse captured" << std::endl;
    } else {
        ShowCursor(TRUE);
        ReleaseCapture();
        std::cout << "[Input] Mouse released" << std::endl;
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
    
    if (mouseCaptured) {
        RECT rect;
        GetClientRect(hwnd, &rect);
        POINT center;
        center.x = (rect.left + rect.right) / 2;
        center.y = (rect.top + rect.bottom) / 2;
        ClientToScreen(hwnd, &center);
        
        POINT currentPos;
        GetCursorPos(&currentPos);
        
        if (currentPos.x != center.x || currentPos.y != center.y) {
            float xoffset = (float)(currentPos.x - center.x);
            float yoffset = (float)(currentPos.y - center.y);
            
            // ИСПРАВЛЕНО: Используем SceneManager для вращения камеры
            auto& sm = SceneManager::Instance();
            sm.RotateCamera(xoffset, yoffset);
            
            SetCursorPos(center.x, center.y);
            lastX = (float)center.x;
            lastY = (float)center.y;
        }
    }
}

void Input::UpdateKeyboardMovement(float deltaTime, HWND hwnd) {
    HWND foreground = GetForegroundWindow();
    if (foreground != hwnd) return;
    
    bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    
    float speed = 15.0f * deltaTime;
    if (shift) speed *= 2.0f;
    
    // ИСПРАВЛЕНО: Используем SceneManager для движения камеры
    auto& sm = SceneManager::Instance();
    CameraComponent* cam = sm.GetMainCamera();
    if (!cam) return;
    
    if (GetAsyncKeyState('W') & 0x8000) {
        // Move camera forward (towards target)
        glm::vec3 forward = glm::normalize(sm.GetCameraTarget() - cam->position);
        forward.y = 0;
        if (glm::length(forward) > 0.01f) forward = glm::normalize(forward);
        cam->target += forward * speed * 5.0f;
        cam->updatePosition();
    }
    if (GetAsyncKeyState('S') & 0x8000) {
        glm::vec3 backward = glm::normalize(cam->position - sm.GetCameraTarget());
        backward.y = 0;
        if (glm::length(backward) > 0.01f) backward = glm::normalize(backward);
        cam->target += backward * speed * 5.0f;
        cam->updatePosition();
    }
    if (GetAsyncKeyState('A') & 0x8000) {
        glm::vec3 right = glm::normalize(glm::cross(cam->position - sm.GetCameraTarget(), glm::vec3(0, 1, 0)));
        cam->target -= right * speed * 5.0f;
        cam->updatePosition();
    }
    if (GetAsyncKeyState('D') & 0x8000) {
        glm::vec3 right = glm::normalize(glm::cross(cam->position - sm.GetCameraTarget(), glm::vec3(0, 1, 0)));
        cam->target += right * speed * 5.0f;
        cam->updatePosition();
    }
    
    // Zoom колесиком мыши
    if (GetAsyncKeyState(VK_UP) & 0x8000) {
        sm.ZoomCamera(-0.5f);
    }
    if (GetAsyncKeyState(VK_DOWN) & 0x8000) {
        sm.ZoomCamera(0.5f);
    }
    
    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
        if (mouseCaptured) SetMouseCapture(false);
    }
    
    static bool rPressed = false;
    if (GetAsyncKeyState('R') & 0x8000) {
        if (!rPressed) {
            rPressed = true;
            sm.ResetCamera();
            std::cout << "[Input] Camera reset" << std::endl;
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
    // Не используется
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
            float yoffset = (float)delta / 120.0f;
            auto& sm = SceneManager::Instance();
            sm.ZoomCamera(yoffset);
        }
    }
}

void Input::Update(float deltaTime) { }

void Input::processInput(float deltaTime) { }

void Input::EnableDebug(bool enable) { 
    debugEnabled = enable; 
}

bool Input::IsMouseCaptured() const { 
    return mouseCaptured; 
}