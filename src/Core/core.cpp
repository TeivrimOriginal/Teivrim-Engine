// core.cpp - FULL FILE WITH INFINITE GRID AND INVERTED Y
#include "core.h"
#include "../Application/application.h"
#include "Render/Win32/RenderUI.h"
#include "../Interface/InterfaceManager.h"
#include "Render/Parser/parser.h"
#include "../Control/Input.h"
#include "SecondComplexity/Project/ProjectManager.h"
#include "SecondComplexity/Asset/AssetManager.h"
#include "SecondComplexity/Scene/SceneManager.h"
#include "../Interface/BufferLayer.h"
#include "SecondComplexity/Icon/IconManager.h"
#include "Otlad.h"
#include "SecondRender.h"
#include <iostream>
#include <string>
#include <thread>
#include <GL/glew.h>
#include <conio.h>
#include <commdlg.h>
#include "../Application/WindowAPIsupport/Win32/InitialWin32.h"
#include "Vulkan.h"

using namespace std;

InterfaceManager* g_uiManager = nullptr;
static bool gridEnabled = true;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            CreateDirectoryA("Config", NULL);
            CreateDirectoryA("Config\\Windows", NULL);
            CreateDirectoryA("System\\Data\\Interface\\Grid", NULL);
            break;
        }
        
        case WM_COMMAND:
            if (LOWORD(wParam) == 1) {
                ProjectManager::Instance().ShowCreateProjectDialog(hwnd);
                return 0;
            } else if (LOWORD(wParam) == 2) {
                OPENFILENAMEA ofn = {0};
                CHAR szFile[MAX_PATH] = "";
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = hwnd;
                ofn.lpstrFile = szFile;
                ofn.nMaxFile = sizeof(szFile);
                ofn.lpstrFilter = "Project Files (*.json)\0*.json\0";
                ofn.lpstrTitle = "Open Project";
                ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
                
                if (GetOpenFileNameA(&ofn)) {
                    std::string projectFile = ofn.lpstrFile;
                    size_t pos = projectFile.find_last_of("\\/");
                    if (pos != std::string::npos) {
                        std::string projectDir = projectFile.substr(0, pos);
                        AssetManager::Instance().LoadProject(projectDir);
                        BufferLayer::Instance().ResetNavigation();
                    }
                }
                return 0;
            } else if (LOWORD(wParam) == 3) {
                DestroyWindow(hwnd);
                return 0;
            }
            break;
            
        case WM_RBUTTONDOWN: {
            int x = LOWORD(lParam), y = HIWORD(lParam);
            if (g_uiManager) g_uiManager->handleRightClick(x, y);
            break;
        }
        
        case WM_LBUTTONDOWN: {
            int x = LOWORD(lParam), y = HIWORD(lParam);
            
            if (BufferLayer::Instance().showContextMenu) {
                BufferLayer::Instance().HandleContextMenuClick(x, y);
                return 0;
            }
            
            if (g_uiManager) {
                g_uiManager->handleClick(x, y);
                g_uiManager->handleMouseDown(x, y);
            }
            break;
        }
        
        case WM_LBUTTONUP: {
            int x = LOWORD(lParam), y = HIWORD(lParam);
            if (g_uiManager) g_uiManager->handleMouseUp(x, y);
            break;
        }
        
        case WM_MOUSEMOVE: {
            int x = LOWORD(lParam), y = HIWORD(lParam);
            if (g_uiManager) g_uiManager->handleMouseMove(x, y);
            break;
        }
        
        case WM_SIZE: {
            if (g_uiManager) {
                int width = LOWORD(lParam), height = HIWORD(lParam);
                if (width > 0 && height > 0) {
                    g_uiManager->updateWindowSize(width, height);
                }
            }
            break;
        }
        
        case WM_KEYDOWN: {
            if (wParam == 'S' && (GetKeyState(VK_CONTROL) & 0x8000)) {
                AssetManager::Instance().SaveProject();
                return 0;
            }
            if (wParam == 'G') {
                gridEnabled = !gridEnabled;
                cout << "[GRID] " << (gridEnabled ? "Enabled" : "Disabled") << endl;
                return 0;
            }
            BufferLayer::Instance().HandleKeyboardInput(wParam);
            break;
        }
        
        case WM_CHAR: {
            BufferLayer::Instance().HandleCharInput((char)wParam);
            break;
        }
        
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
            
        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

Core::Core() {
    cout << "========================================\n";
    cout << "    3D Viewer - Select Render API\n";
    cout << "========================================\n";
    cout << "  [1] OpenGL\n  [2] Vulkan\n";
    cout << "========================================\nChoice: ";
    
    int choice; 
    cin >> choice;
    currentAPI = (choice == 2) ? RenderAPI::VULKAN : RenderAPI::OPENGL;
    cout << (currentAPI == RenderAPI::VULKAN ? "Vulkan selected\n" : "OpenGL selected\n");
    
    ProjectManager::Instance().SetRenderAPI(currentAPI == RenderAPI::VULKAN ? 1 : 0);
    BufferLayer::Instance().SetIconDirectory("System\\Data\\Interface");
}

void Core::setRenderAPI(RenderAPI api) { 
    currentAPI = api; 
}

void Core::initializeRender(InitialWin32* window) {
    currentWindow = window;
    BufferLayer::Instance().SetParentHWND(window->getHWND());
    
    window->onNewProject = [this, window]() {
        ProjectManager::Instance().ShowCreateProjectDialog(window->getHWND(), 
            [this](const std::string& name) {});
    };
    
    window->onOpenProject = [this, window]() {
        OPENFILENAMEA ofn = {0};
        CHAR szFile[MAX_PATH] = "";
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = window->getHWND();
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = "Project Files (*.json)\0*.json\0";
        ofn.lpstrTitle = "Open Project";
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
        
        if (GetOpenFileNameA(&ofn)) {
            std::string projectFile = ofn.lpstrFile;
            size_t pos = projectFile.find_last_of("\\/");
            if (pos != std::string::npos) {
                std::string projectDir = projectFile.substr(0, pos);
                AssetManager::Instance().LoadProject(projectDir);
                BufferLayer::Instance().ResetNavigation();
            }
        }
    };
    
    if (currentAPI == RenderAPI::OPENGL) {
        if (!rendererw.initialize(window)) return;
        shaderProgram = rendererw.initShaders();
        if (shaderProgram == 0) return;
    } else {
        RECT rect; 
        GetClientRect(window->getHWND(), &rect);
        int w = rect.right - rect.left, h = rect.bottom - rect.top;
        if (w <= 0) w = 1280; 
        if (h <= 0) h = 720;
        vulkan = new Vulkan(window->getHWND(), w, h);
        if (!vulkan || !vulkan->isInitialized()) { 
            delete vulkan; 
            vulkan = nullptr; 
            return; 
        }
        vulkan->setup2D(w, h);
    }
    rendererInitialized = true;
}

void Core::renderModel(Camera& camera) {
    if (!rendererInitialized) return;
    if (currentAPI == RenderAPI::OPENGL) {
        // OpenGL rendering would go here
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
        if (vulkan) { 
            vkDeviceWaitIdle(vulkan->getDevice()); 
            delete vulkan; 
            vulkan = nullptr; 
        }
    }
}

bool Core::loadModelFromPath(const std::string& path) {
    if (path.empty()) return false;
    
    ModelParser* newParser = new ModelParser();
    if (!newParser->loadModel(path)) {
        delete newParser;
        return false;
    }
    
    modelPath = path;
    
    std::string modelName = path.substr(path.find_last_of("/\\") + 1);
    
    if (currentAPI == RenderAPI::VULKAN && vulkan) {
        vulkan->addModel(modelName, newParser->getMeshes());
        vulkan->setModelTransform(modelName, glm::scale(glm::mat4(1.0f), glm::vec3(0.01f)));
    }
    
    SceneManager::Instance().AddModel(modelName, newParser);
    
    return true;
}

bool Core::openFileDialogAndLoadModel(HWND hwnd) {
    OPENFILENAMEA ofn = {0};
    CHAR szFile[MAX_PATH] = "";
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "3D Models\0*.obj;*.fbx;*.dae;*.gltf;*.glb\0";
    ofn.lpstrTitle = "Select 3D Model";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    if (GetOpenFileNameA(&ofn)) {
        return loadModelFromPath(ofn.lpstrFile);
    }
    return false;
}

void Core::SetViewportClip(int x, int y, int w, int h) {
    viewportClipEnabled = true;
    clipX = x;
    clipY = y;
    clipW = w;
    clipH = h;
    
    if (currentAPI == RenderAPI::VULKAN && vulkan) {
        vulkan->SetViewportClip(x, y, w, h);
    }
}

void Core::DisableViewportClip() {
    viewportClipEnabled = false;
    
    if (currentAPI == RenderAPI::VULKAN && vulkan) {
        vulkan->DisableViewportClip();
    }
}

void Core::GetViewportClip(int& x, int& y, int& w, int& h) const {
    x = clipX;
    y = clipY;
    w = clipW;
    h = clipH;
}

void Core::GameLoop() {
    std::thread inputThread([]() {
        while (true) {
            std::string input;
            std::getline(std::cin, input);
            
            if (input == "1") {
                Otlad1();
            }
            else if (input == "2") {
                Otlad2();
            }
            else if (input == "3") {
                Otlad3();
            }
            else if (input == "4") {
                Otlad4();
            }
            else if (input == "0") {
                OtladClear();
            }
        }
    });
    inputThread.detach();
    
    Application app;
    if (!app.createApplication()) return;
    
    InitialWin32* win32Window = app.getWindow32();
    if (!win32Window) return;
    
    initializeRender(win32Window);
    
    g_uiManager = new InterfaceManager(this, currentAPI);
    g_uiManager->setWindow(win32Window);
    g_uiManager->setCore(this);
    
    IconManager::Instance().SetIconDirectory("System\\Data\\Interface");
    IconManager::Instance().SetRenderer(&g_uiManager->getRenderer());
    
    RECT rect; 
    GetClientRect(win32Window->getHWND(), &rect);
    int w = rect.right - rect.left, h = rect.bottom - rect.top;
    if (w <= 0) w = 1280; 
    if (h <= 0) h = 720;
    
    g_uiManager->initializeRender(win32Window->getHWND(), w, h);
    
    if (currentAPI == RenderAPI::VULKAN && vulkan) {
        g_uiManager->setVulkan(vulkan);
        IconManager::Instance().SetRenderer(&g_uiManager->getRenderer());
    }
    
    if (currentAPI == RenderAPI::VULKAN && vulkan) {
        SecondRender::Instance().Initialize(vulkan, &g_uiManager->getRenderer(), g_uiManager, w, h);
    } else if (currentAPI == RenderAPI::OPENGL) {
        SecondRender::Instance().Initialize(nullptr, &g_uiManager->getRenderer(), g_uiManager, w, h);
    }
    
    SecondRender::Instance().MarkTestQuadsDirty();
    
    GridConfig gridConf;
    gridConf.enabled = true;
    gridConf.infiniteGrid = true;
    gridConf.gridSpacing = 20.0f;
    gridConf.fadeDistance = 200.0f;
    gridConf.yOffset = 0.0f;
    gridConf.lineColor[0] = 0.3f;
    gridConf.lineColor[1] = 0.3f;
    gridConf.lineColor[2] = 0.35f;
    gridConf.centerLineColor[0] = 0.6f;
    gridConf.centerLineColor[1] = 0.6f;
    gridConf.centerLineColor[2] = 0.7f;
    gridConf.lineThickness = 1.0f;
    SecondRender::Instance().SetGridConfig(gridConf);
    
    win32Window->onResize = [this](int width, int height) {
        if (width <= 0 || height <= 0) return;
        
        if (currentAPI == RenderAPI::VULKAN && vulkan) {
            vulkan->recreateSwapchain();
            vulkan->setup2D(width, height);
            if (g_uiManager) {
                g_uiManager->updateWindowSize(width, height);
                Panel* view3D = g_uiManager->getPanelManager()->get3D();
                if (view3D && view3D->visible && !view3D->collapsed) {
                    SetViewportClip(view3D->getX(), view3D->getY(), view3D->getW(), view3D->getH());
                } else {
                    DisableViewportClip();
                }
            }
            SecondRender::Instance().UpdateScreenSize(width, height);
            SecondRender::Instance().MarkTestQuadsDirty();
        } else if (currentAPI == RenderAPI::OPENGL) {
            glViewport(0, 0, width, height);
            if (g_uiManager) {
                g_uiManager->updateWindowSize(width, height);
                Panel* view3D = g_uiManager->getPanelManager()->get3D();
                if (view3D && view3D->visible && !view3D->collapsed) {
                    SetViewportClip(view3D->getX(), view3D->getY(), view3D->getW(), view3D->getH());
                } else {
                    DisableViewportClip();
                }
            }
            SecondRender::Instance().UpdateScreenSize(width, height);
            SecondRender::Instance().MarkTestQuadsDirty();
        }
    };
    
    SetWindowLongPtr(win32Window->getHWND(), GWLP_WNDPROC, (LONG_PTR)WndProc);
    
    Input input(app, g_uiManager);
    input.EnableDebug(false);
    
    POINT lastMousePos; 
    GetCursorPos(&lastMousePos);
    
    int frameCount = 0; 
    float fpsTimer = 0.0f;
    LARGE_INTEGER freq, lastTime, currentTime;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&lastTime);
    
    Panel* initialView3D = g_uiManager->getPanelManager()->get3D();
    if (initialView3D && initialView3D->visible && !initialView3D->collapsed) {
        SetViewportClip(initialView3D->getX(), initialView3D->getY(), initialView3D->getW(), initialView3D->getH());
    } else {
        DisableViewportClip();
    }
    
    while (!win32Window->shouldClose()) {
        QueryPerformanceCounter(&currentTime);
        float deltaTime = (float)(currentTime.QuadPart - lastTime.QuadPart) / freq.QuadPart;
        if (deltaTime > 0.033f) deltaTime = 0.033f;
        lastTime = currentTime;
        
        fpsTimer += deltaTime; 
        frameCount++;
        if (fpsTimer >= 1.0f) {
            char title[256];
            sprintf_s(title, "%s 3D Viewer | FPS: %d | Grid: %s", 
                      currentAPI == RenderAPI::VULKAN ? "Vulkan" : "OpenGL", 
                      frameCount,
                      gridEnabled ? "ON" : "OFF");
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
            input.Update(deltaTime);
            
            RECT clientRect; 
            GetClientRect(win32Window->getHWND(), &clientRect);
            int cw = clientRect.right - clientRect.left, ch = clientRect.bottom - clientRect.top;
            if (cw <= 0) cw = 1280; 
            if (ch <= 0) ch = 720;
            
            Panel* currentView3D = g_uiManager->getPanelManager()->get3D();
            if (currentView3D && currentView3D->visible && !currentView3D->collapsed) {
                int vx = currentView3D->getX();
                int vy = currentView3D->getY();
                int vw = currentView3D->getW();
                int vh = currentView3D->getH();
                if (vx != clipX || vy != clipY || vw != clipW || vh != clipH) {
                    SetViewportClip(vx, vy, vw, vh);
                    SecondRender::Instance().UpdateViewportRect();
                    SecondRender::Instance().MarkTestQuadsDirty();
                }
            } else {
                if (viewportClipEnabled) {
                    DisableViewportClip();
                    SecondRender::Instance().UpdateViewportRect();
                    SecondRender::Instance().MarkTestQuadsDirty();
                }
            }
            
            if (currentAPI == RenderAPI::VULKAN && vulkan) {
                ProcessOtladCommands(vulkan, g_uiManager);
                
                vulkan->beginFrame();
                
                vulkan->setViewMatrix(app.getCamera().GetViewMatrix());
                
                // СОЗДАЕМ ПРОЕКЦИЮ С ИНВЕРСИЕЙ Y ДЛЯ VULKAN
                glm::mat4 proj = glm::perspective(glm::radians(app.getCamera().GetZoom()), 
                                         (float)cw/ch, 0.1f, 1000.0f);
                vulkan->setProjectionMatrix(proj);
                
                SecondRender::Instance().RenderBackground();
                vulkan->renderBackground();
                
                if (currentView3D && currentView3D->visible && !currentView3D->collapsed) {
                    SetViewportClip(currentView3D->getX(), currentView3D->getY(), 
                                    currentView3D->getW(), currentView3D->getH());
                    vulkan->SetViewportClip(currentView3D->getX(), currentView3D->getY(),
                                            currentView3D->getW(), currentView3D->getH());
                } else {
                    DisableViewportClip();
                    vulkan->DisableViewportClip();
                }
                
                vulkan->renderScene();
                
                if (gridEnabled) {
                    DisableViewportClip();
                    vulkan->DisableViewportClip();
                    
                    // СОЗДАЕМ ПРОЕКЦИЮ С ИНВЕРСИЕЙ Y ДЛЯ ГРИД
                    glm::mat4 projGrid = glm::perspective(glm::radians(app.getCamera().GetZoom()), 
                                                 (float)cw/ch, 0.1f, 1000.0f);
                    projGrid[1][1] *= -1;  // ИНВЕРТИРУЕМ Y ДЛЯ ГРИД
                    
                    SecondRender::Instance().SetCamera(
                        app.getCamera().GetViewMatrix(),
                        projGrid,
                        app.getCamera().GetPosition()
                    );
                    
                    SecondRender::Instance().RenderInfiniteGrid();
                    
                    if (currentView3D && currentView3D->visible && !currentView3D->collapsed) {
                        SetViewportClip(currentView3D->getX(), currentView3D->getY(), 
                                        currentView3D->getW(), currentView3D->getH());
                        vulkan->SetViewportClip(currentView3D->getX(), currentView3D->getY(),
                                                currentView3D->getW(), currentView3D->getH());
                    }
                }
                
                DisableViewportClip();
                vulkan->DisableViewportClip();
                g_uiManager->renderStatic();
                
                SecondRender::Instance().RenderOverlay();
                vulkan->renderOverlay();
                
                vulkan->endFrame();
                vulkan->present();
            } 
            else if (currentAPI == RenderAPI::OPENGL) {
                glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                
                SecondRender::Instance().RenderBackground();
                
                if (currentView3D && currentView3D->visible && !currentView3D->collapsed) {
                    glEnable(GL_SCISSOR_TEST);
                    glScissor(currentView3D->getX(), ch - (currentView3D->getY() + currentView3D->getH()),
                              currentView3D->getW(), currentView3D->getH());
                }
                
                // OpenGL НЕ ТРЕБУЕТ ИНВЕРСИИ Y
                if (gridEnabled) {
                    if (currentView3D && currentView3D->visible && !currentView3D->collapsed) {
                        glDisable(GL_SCISSOR_TEST);
                    }
                    
                    glm::mat4 projOGL = glm::perspective(glm::radians(app.getCamera().GetZoom()), 
                                                (float)cw/ch, 0.1f, 1000.0f);
                    // OpenGL уже использует Y вверх, инверсия не нужна
                    
                    SecondRender::Instance().SetCamera(
                        app.getCamera().GetViewMatrix(),
                        projOGL,
                        app.getCamera().GetPosition()
                    );
                    SecondRender::Instance().RenderInfiniteGrid();
                    
                    if (currentView3D && currentView3D->visible && !currentView3D->collapsed) {
                        glEnable(GL_SCISSOR_TEST);
                    }
                }
                
                if (currentView3D && currentView3D->visible && !currentView3D->collapsed) {
                    glDisable(GL_SCISSOR_TEST);
                }
                
                g_uiManager->renderStatic();
                
                SecondRender::Instance().RenderOverlay();
                
                win32Window->swapBuffers();
            }
        }
        lastMousePos = currentMousePos;
        if (deltaTime < 0.016f) Sleep(1);
    }
    
    delete g_uiManager; 
    g_uiManager = nullptr;
    cleanupRender();
}

void Core::settingUpRender() {}

void Core::ParserToRender() {}