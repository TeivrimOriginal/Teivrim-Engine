#include "core.h"
#include "../Application/application.h"
#include "Render/Win32/rendererw.h"
#include "Render/Win32/RenderUI.h"
#include "../Interface/InterfaceManager.h"
#include "Render/Parser/parser.h"
#include "../Control/Input.h"
#include <iostream>
#include <string>
#include <GL/glew.h>
#include <conio.h>
#include <commdlg.h>
#include "../Application/WindowAPIsupport/Win32/InitialWin32.h"

using namespace std;

InterfaceManager* g_uiManager = nullptr;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_LBUTTONDOWN: {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            if (g_uiManager) {
                g_uiManager->handleMouseDown(x, y);
                g_uiManager->handleClick(x, y);
            }
            break;
        }
        case WM_LBUTTONUP: {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            if (g_uiManager) {
                g_uiManager->handleMouseUp(x, y);
            }
            break;
        }
        case WM_MOUSEMOVE: {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            if (g_uiManager) {
                g_uiManager->handleMouseMove(x, y);
            }
            break;
        }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void Core::settingUpRender() {
    std::cout << "Render setup complete" << std::endl;
}

void Core::ParserToRender() {
}

bool Core::loadModelFromPath(const std::string& path) {
    if (path.empty()) {
        std::cerr << "Путь к модели пустой" << std::endl;
        return false;
    }

    if (!modelParser.loadModel(path)) {
        std::cerr << "Ошибка загрузки модели: " << path << std::endl;
        return false;
    }

    modelPath = path;
    modelLoaded = true;
    needsOptimize = true;

    std::cout << "Модель успешно загружена: " << path << std::endl;
    return true;
}

bool Core::openFileDialogAndLoadModel(HWND hwnd) {
    OPENFILENAMEA ofn;
    CHAR szFile[MAX_PATH] = "";

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "3D Models (OBJ, FBX, DAE, GLTF, GLB)\0*.obj;*.fbx;*.dae;*.gltf;*.glb\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    ofn.lpstrTitle = "Выберите 3D модель";

    if (GetOpenFileNameA(&ofn)) {
        if (loadModelFromPath(ofn.lpstrFile)) {
            return true;
        }
    }

    return false;
}

void Core::GameLoop() {
    Application app;

    if (!app.createApplication()) {
        std::cerr << "Application creation failed!" << std::endl;
        return;
    }

    InitialWin32* win32Window = app.getWindow32();
    if (!win32Window) {
        std::cerr << "Error: Win32 window is null" << std::endl;
        return;
    }

    if (!rendererw.initialize(win32Window)) {
        std::cerr << "Error: Failed to initialize RendererW" << std::endl;
        return;
    }

    shaderProgram = rendererw.initShaders();
    if (shaderProgram == 0) {
        std::cerr << "Error: Failed to create shader program" << std::endl;
        return;
    }

    g_uiManager = new InterfaceManager(this);
    g_uiManager->setWindow(win32Window);
    SetWindowLongPtr(win32Window->getHWND(), GWLP_WNDPROC, (LONG_PTR)WndProc);

    Input input(app, g_uiManager);
    POINT lastMousePos = {0, 0};
    GetCursorPos(&lastMousePos);

    int frameCount = 0;
    float fpsTimer = 0.0f;

    while (!win32Window->shouldClose()) {
        static float lastFrame = 0.0f;
        float currentFrame = GetTickCount() / 1000.0f;
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        fpsTimer += deltaTime;
        frameCount++;
        if (fpsTimer >= 1.0f) {
            char title[256];
            sprintf_s(title, "FPS: %d", frameCount);
            SetWindowTextA(win32Window->getHWND(), title);
            frameCount = 0;
            fpsTimer = 0.0f;
        }

        win32Window->pollEvents();

        POINT currentMousePos;
        GetCursorPos(&currentMousePos);
        ScreenToClient(win32Window->getHWND(), &currentMousePos);

        if (needsOptimize && modelLoaded && shaderProgram != 0) {
            rendererw.optimize(modelParser, shaderProgram);
            needsOptimize = false;
        }

        if (!isStart) {
            // Блокировка движения камеры при перетаскивании панели
            if (g_uiManager && g_uiManager->isBlockingRender()) {
                input.processMouseWin32((float)currentMousePos.x, (float)currentMousePos.y);
            } else {
                input.processMouseWin32((float)currentMousePos.x, (float)currentMousePos.y);
                input.processInputWin32(deltaTime, win32Window->getHWND());
            }

            rendererw.setAnimateModel(false);

            glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            if (modelLoaded && g_uiManager) {
                Panel* view3D = g_uiManager->getPanelManager()->get3D();
                if (view3D && view3D->isVisible()) {
                    RECT rect;
                    GetClientRect(win32Window->getHWND(), &rect);
                    int winH = rect.bottom - rect.top;
                    int viewY = winH - (view3D->getY() + view3D->getH());
                    
                    glViewport(view3D->getX(), viewY, view3D->getW(), view3D->getH());
                    glScissor(view3D->getX(), viewY, view3D->getW(), view3D->getH());
                    glEnable(GL_SCISSOR_TEST);
                    
                    rendererw.renderModel(modelParser, shaderProgram, app.getCamera());
                    
                    glDisable(GL_SCISSOR_TEST);
                } else {
                    RECT rect;
                    GetClientRect(win32Window->getHWND(), &rect);
                    glViewport(0, 0, rect.right - rect.left, rect.bottom - rect.top);
                    rendererw.renderModel(modelParser, shaderProgram, app.getCamera());
                }
            }
        }

        g_uiManager->renderStatic();

        HDC hdc = GetDC(win32Window->getHWND());
        SwapBuffers(hdc);
        ReleaseDC(win32Window->getHWND(), hdc);

        lastMousePos = currentMousePos;
        Sleep(0);
    }

    delete g_uiManager;
    g_uiManager = nullptr;

    glDeleteProgram(shaderProgram);
    rendererw.cleanup();
}