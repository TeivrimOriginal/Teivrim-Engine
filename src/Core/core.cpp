#include "core.h"
#include "../Application/application.h"
#include "Render/Win32/rendererw.h"
#include "Render/Win32/RenderUI.h"
#include "../Interface/InterfaceManager.h"
#include "parser.h"
#include "../Control/Input.h"
#include <iostream>
#include <String>
#include <GL/glew.h>
#include <conio.h>
#include "../Application/WindowAPIsupport/Win32/InitialWin32.h"

using namespace std;

string model;

// Глобальные переменные
InterfaceManager* g_uiManager = nullptr;

// Обработчик кликов мыши для Win32
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_LBUTTONDOWN: {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            
            if (g_uiManager) {
                g_uiManager->handleClick(x, y);
                std::cout << "Click handled at: " << x << ", " << y << std::endl;
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

void Core::GameLoop() {
    Application app;
    cout << "poiza";
    
    string model;
    cout << "введите модель которую хотите импортировать" << endl;
    cin >> model;
    cout << endl;
    
    cout << "poiza";
    
    if (!app.createApplication()) {
        std::cerr << "Application creation failed!" << std::endl;
        return;
    }
    
    cout << "PIZDAAA TI DOSHEL" << endl;

    InitialWin32* win32Window = app.getWindow32();
    if (!win32Window) {
        std::cerr << "Error: Win32 window is null" << std::endl;
        return;
    }

    RendererW rendererw;
    if (!rendererw.initialize(win32Window)) {
        std::cerr << "Error: Failed to initialize RendererW" << std::endl;
        return;
    }

    GLuint shaderProgram = rendererw.initShaders();
    if (shaderProgram == 0) {
        std::cerr << "Error: Failed to create shader program" << std::endl;
        return;
    }

    ModelParser modelParser;
    if (!modelParser.loadModel(model)) {
        std::cerr << "Error: Failed to load model" << std::endl;
        glDeleteProgram(shaderProgram);
        return;
    }

    g_uiManager = new InterfaceManager(this);
    g_uiManager->setWindow(win32Window);
    SetWindowLongPtr(win32Window->getHWND(), GWLP_WNDPROC, (LONG_PTR)WndProc);

    rendererw.optimize(modelParser, shaderProgram);

    Input input(app);
    POINT lastMousePos = {0, 0};
    GetCursorPos(&lastMousePos);

    int frameCount = 0;
    float fpsTimer = 0.0f;

    while (!win32Window->shouldClose()) {
        static float lastFrame = 0.0f;
        float currentFrame = GetTickCount() / 1000.0f;
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // FPS счетчик
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

        if (!isStart) {
            input.processMouseWin32((float)currentMousePos.x, (float)currentMousePos.y);
            input.processInputWin32(deltaTime, win32Window->getHWND());

            rendererw.setAnimateModel(false);

            glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            rendererw.renderModel(modelParser, shaderProgram, app.getCamera());
        } else {
            std::cout << "cicl ostanovlen " << std::endl;
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