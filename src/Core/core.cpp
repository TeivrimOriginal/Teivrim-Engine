// core.cpp - ПОЛНЫЙ ФАЙЛ С ПОСТ-РЕНДЕРОМ
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
#include "PublicAPI.h"
#include <iostream>
#include <string>
#include <thread>
#include <GL/glew.h>
#include <conio.h>
#include <commdlg.h>
#include "../Application/WindowAPIsupport/Win32/InitialWin32.h"
#include "Vulkan.h"
#include "PostRender.h"

using namespace std;

InterfaceManager* g_uiManager = nullptr;
static bool gridEnabled = true;
static Input* g_input = nullptr;

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
        
        case WM_MOUSEWHEEL: {
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            if (g_input) g_input->processMouseWheel(delta, hwnd);
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
            if (g_uiManager && g_uiManager->getObjectUI().isAnyInputActive()) {
                g_uiManager->getObjectUI().handleKeyboardInput(wParam, 0);
                return 0;
            }
            
            if (wParam == 'S' && (GetKeyState(VK_CONTROL) & 0x8000)) {
                AssetManager::Instance().SaveProject();
                return 0;
            }
            if (wParam == 'G') {
                gridEnabled = !gridEnabled;
                if (g_uiManager && g_uiManager->getCore() && g_uiManager->getCore()->getVulkan()) {
                    g_uiManager->getCore()->getVulkan()->setGridEnabled(gridEnabled);
                }
                cout << "[GRID] " << (gridEnabled ? "Enabled" : "Disabled") << endl;
                return 0;
            }
            BufferLayer::Instance().HandleKeyboardInput(wParam);
            break;
        }
        
        case WM_CHAR: {
            if (g_uiManager && g_uiManager->getObjectUI().isAnyInputActive()) {
                g_uiManager->getObjectUI().handleKeyboardInput(0, (char)wParam);
                return 0;
            }
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
    
    static int modelCounter = 0;
    modelCounter++;
    
    // Извлекаем имя файла без расширения
    std::string filename = path.substr(path.find_last_of("/\\") + 1);
    size_t dotPos = filename.find_last_of(".");
    if (dotPos != std::string::npos) {
        filename = filename.substr(0, dotPos);
    }
    
    std::string uniqueName = std::to_string(modelCounter) + "_" + filename;
    
    std::cout << "[Core] Loading model with unique name: " << uniqueName << std::endl;
    
    ModelParser* newParser = new ModelParser();
    if (!newParser->loadModel(path)) {
        delete newParser;
        return false;
    }
    
    modelPath = path;
    
    if (currentAPI == RenderAPI::VULKAN && vulkan) {
        vulkan->addModel(uniqueName, newParser->getMeshes());
        vulkan->setModelTransform(uniqueName, glm::mat4(1.0f));
    }
    
    auto& sm = SceneManager::Instance();
    int objId = sm.CreateObjectScene(uniqueName);
    sm.SetModelParser(objId, newParser);
    sm.LoadModelToScene(objId, path);
    sm.SetPositionScene(objId, 0.0f, 0.0f, 0.0f);
    
    std::cout << "[Core] Loaded model: " << uniqueName << " (ObjID: " << objId << ")" << std::endl;
    
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

void Core::settingUpRender() {}

void Core::ParserToRender() {}

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
            else if (input == "scene") {
                auto& sm = SceneManager::Instance();
                std::cout << "\n=== Scene Objects ===" << std::endl;
                for (const auto& obj : sm.GetAllObjectsScene()) {
                    std::cout << "ID: " << obj.id << " | " << obj.name 
                              << " | Pos: " << obj.posX << ", " << obj.posY << ", " << obj.posZ
                              << " | Scale: " << obj.scaleX << ", " << obj.scaleY << ", " << obj.scaleZ
                              << " | Visible: " << (obj.isVisible ? "Y" : "N") 
                              << " | Loaded: " << (obj.isLoaded ? "Y" : "N") << std::endl;
                }
                std::cout << "=====================" << std::endl;
            }
            else if (input == "analyze") {
                OtladAnalyze();
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
        
        vulkan->setGridSpacing(20.0f);
        vulkan->setGridEnabled(gridEnabled);
    }
    
    if (currentAPI == RenderAPI::VULKAN && vulkan) {
        SecondRender::Instance().Initialize(vulkan, &g_uiManager->getRenderer(), g_uiManager, w, h);
    } else if (currentAPI == RenderAPI::OPENGL) {
        SecondRender::Instance().Initialize(nullptr, &g_uiManager->getRenderer(), g_uiManager, w, h);
    }
    
    SecondRender::Instance().MarkTestQuadsDirty();
    
    win32Window->onResize = [this](int width, int height) {
        if (width <= 0 || height <= 0) return;
        
        if (currentAPI == RenderAPI::VULKAN && vulkan && vulkan->isInitialized()) {
            vulkan->recreateSwapchain();
            vulkan->setup2D(width, height);
            if (g_uiManager) {
                g_uiManager->updateWindowSize(width, height);
                Panel* view3D = g_uiManager->getPanelManager()->get3D();
                if (view3D && view3D->visible && !view3D->collapsed) {
                    SetViewportClip(view3D->getX(), view3D->getY(), view3D->getW(), view3D->getH());
                    vulkan->SetViewportClip(view3D->getX(), view3D->getY(), view3D->getW(), view3D->getH());
                } else {
                    DisableViewportClip();
                    vulkan->DisableViewportClip();
                }
            }
            SecondRender::Instance().UpdateScreenSize(width, height);
            SecondRender::Instance().MarkTestQuadsDirty();
            
            auto& sm = SceneManager::Instance();
            sm.UpdateCameraAspect((float)width / (float)height);
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
    g_input = &input;
    
    POINT lastMousePos; 
    GetCursorPos(&lastMousePos);
    
    int frameCount = 0; 
    float fpsTimer = 0.0f;
    LARGE_INTEGER freq, lastTime, currentTime;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&lastTime);
    
    auto& sm = SceneManager::Instance();
    if (sm.GetMainCameraID() == 0) {
        sm.CreateCamera("MainCamera");
    }
    sm.SetCameraDistance(20.0f);
    sm.SetCameraFov(60.0f);
    sm.SetCameraTarget(glm::vec3(0.0f, 0.0f, 0.0f));
    sm.UpdateCameraAspect((float)w / (float)h);
    
    // ========== НАСТРОЙКА ПОСТ-РЕНДЕРА (ОДИН РАЗ) ==========
    if (vulkan && vulkan->GetPostRender()) {
        vulkan->GetPostRender()->SetOutlineThickness(3);  // 3 пикселя толщина контура
        vulkan->GetPostRender()->SetOutlineColor(1.0f, 0.5f, 0.0f);  // Оранжевый
        vulkan->GetPostRender()->SetEnabled(true);
    }
    
    std::cout << "[Core] GameLoop started" << std::endl;
    
    while (!win32Window->shouldClose()) {
        QueryPerformanceCounter(&currentTime);
        float deltaTime = (float)(currentTime.QuadPart - lastTime.QuadPart) / freq.QuadPart;
        if (deltaTime > 0.033f) deltaTime = 0.033f;
        lastTime = currentTime;
        
        fpsTimer += deltaTime; 
        frameCount++;
        if (fpsTimer >= 1.0f) {
            char title[256];
            sprintf_s(title, "%s 3D Viewer | FPS: %d | Grid: %s | Objects: %zu", 
                      currentAPI == RenderAPI::VULKAN ? "Vulkan" : "OpenGL", 
                      frameCount,
                      gridEnabled ? "ON" : "OFF",
                      sm.GetAllObjectsScene().size());
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
            
            sm.UpdateCameraAspect((float)cw / (float)ch);
            
            if (currentAPI == RenderAPI::VULKAN && vulkan && vulkan->isInitialized()) {
                ProcessOtladCommands(vulkan, g_uiManager);
                
                glm::mat4 viewMat = sm.GetViewMatrix();
                glm::mat4 projMat = sm.GetProjectionMatrix();
                glm::vec3 camPos = sm.GetCameraPosition();
                
                vulkan->setViewMatrix(viewMat);
                vulkan->setProjectionMatrix(projMat);
                
                vulkan->beginFrame();
                
                Panel* currentView3D = g_uiManager->getPanelManager()->get3D();
                if (currentView3D && currentView3D->visible && !currentView3D->collapsed) {
                    SetViewportClip(currentView3D->getX(), currentView3D->getY(), 
                                    currentView3D->getW(), currentView3D->getH());
                    vulkan->SetViewportClip(currentView3D->getX(), currentView3D->getY(),
                                            currentView3D->getW(), currentView3D->getH());
                } else {
                    DisableViewportClip();
                    vulkan->DisableViewportClip();
                }
                
                sm.UpdateAllMatrices();
                
                // ========== УСТАНОВКА ID ВЫБРАННОГО ОБЪЕКТА ДЛЯ ПОСТ-РЕНДЕРА ==========
if (vulkan) {
    vulkan->SetSelectedObjectID(99);  // Тестовый ID
}
                
                // ========== ОБНОВЛЕНИЕ ТРАНСФОРМАЦИЙ МОДЕЛЕЙ ==========
                for (const auto& obj : sm.GetAllObjectsScene()) {
                    if (obj.isLoaded && !obj.name.empty()) {
                        glm::mat4 transform = glm::mat4(1.0f);
                        transform = glm::translate(transform, glm::vec3(obj.posX, obj.posY, obj.posZ));
                        transform = glm::scale(transform, glm::vec3(obj.scaleX, obj.scaleY, obj.scaleZ));
                        
                        if (vulkan) {
                            vulkan->setModelTransform(obj.name, transform);
                        }
                    }
                }
                
                // ========== РЕНДЕР ВСЕХ МОДЕЛЕЙ ==========
                if (vulkan) {
                    vulkan->renderAllModels();
                }
                
                DisableViewportClip();
                if (vulkan) {
                    vulkan->DisableViewportClip();
                }
                
                g_uiManager->renderStatic();
                SecondRender::Instance().RenderOverlay();
                if (vulkan) {
                    vulkan->renderOverlay();
                }
                
                vulkan->endFrame();
                vulkan->present();
            } 
            else if (currentAPI == RenderAPI::OPENGL) {
                glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                
                SecondRender::Instance().RenderBackground();
                
                Panel* currentView3D = g_uiManager->getPanelManager()->get3D();
                if (currentView3D && currentView3D->visible && !currentView3D->collapsed) {
                    glEnable(GL_SCISSOR_TEST);
                    glScissor(currentView3D->getX(), ch - (currentView3D->getY() + currentView3D->getH()),
                              currentView3D->getW(), currentView3D->getH());
                }
                
                if (gridEnabled) {
                    if (currentView3D && currentView3D->visible && !currentView3D->collapsed) {
                        glDisable(GL_SCISSOR_TEST);
                    }
                    
                    glm::mat4 viewMat = sm.GetViewMatrix();
                    glm::mat4 projMat = sm.GetProjectionMatrix();
                    glm::vec3 camPos = sm.GetCameraPosition();
                    
                    SecondRender::Instance().SetCamera(viewMat, projMat, camPos);
                    SecondRender::Instance().SetZoomLevel(glm::length(camPos) / 20.0f);
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
    
    sm.SaveScene("Config/last_scene.json");
    
    delete g_uiManager; 
    g_uiManager = nullptr;
    cleanupRender();
}