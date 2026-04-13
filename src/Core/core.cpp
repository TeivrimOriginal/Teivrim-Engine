#include "core.h"
#include "../Application/application.h"
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
#include "Vulkan.h"

using namespace std;

InterfaceManager* g_uiManager = nullptr;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_MOUSEMOVE: {
            int x = LOWORD(lParam), y = HIWORD(lParam);
            if (g_uiManager) {
                if (msg == WM_LBUTTONDOWN) {
                    g_uiManager->handleMouseDown(x, y);
                    g_uiManager->handleClick(x, y);
                }
                else if (msg == WM_LBUTTONUP) g_uiManager->handleMouseUp(x, y);
                else g_uiManager->handleMouseMove(x, y);
            }
            break;
        }
        case WM_SIZE: {
            if (g_uiManager) {
                int width = LOWORD(lParam);
                int height = HIWORD(lParam);
                if (width > 0 && height > 0) {
                    g_uiManager->updateWindowSize(width, height);
                }
            }
            break;
        }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

Core::Core() {
    std::cout << "========================================\n";
    std::cout << "    3D Viewer - Select Render API\n";
    std::cout << "========================================\n";
    std::cout << "  [1] OpenGL (Fully Working)\n";
    std::cout << "  [2] Vulkan (Experimental)\n";
    std::cout << "========================================\n";
    std::cout << "Choice: ";
    
    int choice;
    std::cin >> choice;
    currentAPI = (choice == 2) ? RenderAPI::VULKAN : RenderAPI::OPENGL;
    std::cout << (currentAPI == RenderAPI::VULKAN ? "Vulkan selected\n" : "OpenGL selected\n");
}

void Core::setRenderAPI(RenderAPI api) {
    currentAPI = api;
    std::cout << (currentAPI == RenderAPI::VULKAN ? "Switched to Vulkan\n" : "Switched to OpenGL\n");
}

void Core::initializeRender(InitialWin32* window) {
    currentWindow = window;
    if (currentAPI == RenderAPI::OPENGL) {
        if (!rendererw.initialize(window)) {
            std::cerr << "Failed to initialize OpenGL renderer" << std::endl;
            return;
        }
        shaderProgram = rendererw.initShaders();
        if (shaderProgram == 0) {
            std::cerr << "Failed to create shader program" << std::endl;
            return;
        }
        std::cout << "OpenGL renderer initialized" << std::endl;
    } 
    else {
        RECT rect;
        GetClientRect(window->getHWND(), &rect);
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;
        if (width <= 0) width = 1280;
        if (height <= 0) height = 720;

        vulkan = new Vulkan(window->getHWND(), width, height);
        if (!vulkan || !vulkan->isInitialized()) {
            std::cerr << "Failed to initialize Vulkan renderer" << std::endl;
            delete vulkan;
            vulkan = nullptr;
            return;
        }
        vulkan->setup2D(width, height);
        std::cout << "Vulkan renderer initialized" << std::endl;
    }
    rendererInitialized = true;
}

void Core::renderModel(Camera& camera) {
    if (!rendererInitialized || !modelLoaded) return;
    
    if (currentAPI == RenderAPI::OPENGL) {
        if (needsOptimize) {
            rendererw.optimize(modelParser, shaderProgram);
            needsOptimize = false;
        }
        rendererw.renderModel(modelParser, shaderProgram, camera);
    }
}

void Core::cleanupRender() {
    if (currentAPI == RenderAPI::OPENGL) {
        rendererw.cleanup();
        if (shaderProgram) {
            glDeleteProgram(shaderProgram);
            shaderProgram = 0;
        }
    } else {
        delete vulkan;
        vulkan = nullptr;
    }
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

    if (currentAPI == RenderAPI::VULKAN && vulkan) {
        vulkan->loadModel(modelParser.getMeshes());
    }

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
        return loadModelFromPath(ofn.lpstrFile);
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

    initializeRender(win32Window);

    g_uiManager = new InterfaceManager(this, currentAPI);
    g_uiManager->setWindow(win32Window);

    RECT rect;
    GetClientRect(win32Window->getHWND(), &rect);
    int windowWidth = rect.right - rect.left;
    int windowHeight = rect.bottom - rect.top;

    if (windowWidth <= 0) windowWidth = 1280;
    if (windowHeight <= 0) windowHeight = 720;

    g_uiManager->initializeRender(win32Window->getHWND(), windowWidth, windowHeight);

    if (currentAPI == RenderAPI::VULKAN && vulkan) {
        g_uiManager->setVulkan(vulkan);
    }

    SetWindowLongPtr(win32Window->getHWND(), GWLP_WNDPROC, (LONG_PTR)WndProc);

    Input input(app, g_uiManager);

    POINT lastMousePos = {0, 0};
    GetCursorPos(&lastMousePos);

    int frameCount = 0;
    float fpsTimer = 0.0f;

    while (!win32Window->shouldClose()) {
        static float lastFrame = 0.0f;
        float currentFrame = (float)GetTickCount() / 1000.0f;
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        if (deltaTime > 0.033f) deltaTime = 0.033f;

        fpsTimer += deltaTime;
        frameCount++;

        if (fpsTimer >= 1.0f) {
            char title[256];
            if (currentAPI == RenderAPI::VULKAN)
                sprintf_s(title, "Vulkan 3D Viewer | FPS: %d", frameCount);
            else
                sprintf_s(title, "OpenGL 3D Viewer | FPS: %d", frameCount);

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

            RECT clientRect;
            GetClientRect(win32Window->getHWND(), &clientRect);
            int clientWidth = clientRect.right - clientRect.left;
            int clientHeight = clientRect.bottom - clientRect.top;

            if (clientWidth <= 0) clientWidth = 1280;
            if (clientHeight <= 0) clientHeight = 720;

            if (currentAPI == RenderAPI::VULKAN && vulkan) {
                vulkan->beginFrame();
                
                if (modelLoaded) {
                    vulkan->setViewMatrix(app.getCamera().GetViewMatrix());
                    vulkan->setProjectionMatrix(glm::perspective(glm::radians(45.0f), (float)clientWidth/clientHeight, 0.1f, 1000.0f));
                    vulkan->renderModel();
                }
                
                g_uiManager->renderStatic();
                vulkan->endFrame();
                vulkan->present();
            }
            else if (currentAPI == RenderAPI::OPENGL) {
                glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                if (modelLoaded && g_uiManager) {
                    Panel* view3D = g_uiManager->getPanelManager()->get3D();
                    if (view3D && view3D->visible) {
                        int winH = clientHeight;
                        int viewY = winH - (view3D->getY() + view3D->getH());
                        glViewport(view3D->getX(), viewY, view3D->getW(), view3D->getH());
                        glScissor(view3D->getX(), viewY, view3D->getW(), view3D->getH());
                        glEnable(GL_SCISSOR_TEST);
                        renderModel(app.getCamera());
                        glDisable(GL_SCISSOR_TEST);
                    } else {
                        glViewport(0, 0, clientWidth, clientHeight);
                        renderModel(app.getCamera());
                    }
                }
                g_uiManager->renderStatic();
                win32Window->swapBuffers();
            }
        }

        lastMousePos = currentMousePos;
        Sleep(1);
    }

    delete g_uiManager;
    g_uiManager = nullptr;
    cleanupRender();
}