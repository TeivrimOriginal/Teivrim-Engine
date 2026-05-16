// Input.cpp - ИСПРАВЛЕННЫЙ (без дубликатов)
#include "Input.h"
#include "../Interface/InterfaceManager.h"
#include "../Core/SecondComplexity/Scene/SceneManager.h"
#include <windows.h>
#include <iostream>

Input::Input(Application& app, InterfaceManager* uiManager) 
    : m_app(app), m_uiManager(uiManager), m_captured(false), m_lastX(0), m_lastY(0) {
}

Input::~Input() {
}

void Input::processMouseWin32(float x, float y) {
    if (!m_captured) return;
    
    float dx = x - m_lastX;
    float dy = y - m_lastY;
    
    if (dx != 0 || dy != 0) {
        auto& sm = SceneManager::Instance();
        sm.RotateCamera(dx, dy);
    }
    
    m_lastX = x;
    m_lastY = y;
}

void Input::processInputWin32(float deltaTime, HWND hwnd) {
    UpdateKeyboardMovement(deltaTime, hwnd);
    UpdateMouseCapture(hwnd);
}

void Input::Update(float deltaTime) {
    // Дополнительная логика обновления
}

void Input::UpdateKeyboardMovement(float deltaTime, HWND hwnd) {
    auto& sm = SceneManager::Instance();
    float speed = 10.0f * deltaTime;
    
    if (GetAsyncKeyState('W') & 0x8000) {
        sm.ZoomCamera(-speed);
    }
    if (GetAsyncKeyState('S') & 0x8000) {
        sm.ZoomCamera(speed);
    }
    if (GetAsyncKeyState('A') & 0x8000) {
        sm.PanCamera(-speed, 0);
    }
    if (GetAsyncKeyState('D') & 0x8000) {
        sm.PanCamera(speed, 0);
    }
    if (GetAsyncKeyState('Q') & 0x8000) {
        sm.PanCamera(0, speed);
    }
    if (GetAsyncKeyState('E') & 0x8000) {
        sm.PanCamera(0, -speed);
    }
}

// Только ОДНО определение метода UpdateMouseCapture
void Input::UpdateMouseCapture(HWND hwnd) {
    if (GetAsyncKeyState(VK_RBUTTON) & 0x8000) {
        if (!m_captured) {
            m_captured = true;
            SetCapture(hwnd);
            
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(hwnd, &pt);
            m_lastX = (float)pt.x;
            m_lastY = (float)pt.y;
            
            ShowCursor(FALSE);
            std::cout << "[Input] Mouse captured" << std::endl;
        }
    } else {
        if (m_captured) {
            m_captured = false;
            ReleaseCapture();
            ShowCursor(TRUE);
            std::cout << "[Input] Mouse released" << std::endl;
        }
    }
}

// Только ОДНО определение метода processMouseWheel
void Input::processMouseWheel(int delta, HWND hwnd) {
    auto& sm = SceneManager::Instance();
    float yoffset = (float)delta / 120.0f;
    sm.ZoomCamera(yoffset * 2.0f);
}