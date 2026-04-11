#include "InterfaceManager.h"
#include "../Core/core.h"  // нужно добавить для использования RenderAPI
#include "../Core/Vulkan.h"
#include <iostream>

// ... остальной код без изменений ...

InterfaceManager::InterfaceManager(Core* corePtr, RenderAPI api) 
    : window(nullptr), core(corePtr), currentAPI(api), startupActive(false),
      renderer(api == RenderAPI::OPENGL ? RenderAPIType::OPENGL : RenderAPIType::VULKAN)
{
    panels = new PanelManager();
    
    int sw = 1280, sh = 720;
    
    panels->add("TopBar", 0, 0, sw, 40);
    panels->add("Hierarchy", 0, 40, 220, sh - 40);
    panels->add("Inspector", sw - 260, 40, 260, sh - 40);
    panels->add("3D Viewport", 220, 40, sw - 480, sh - 40, true);
    panels->add("Console", 220, sh - 150, sw - 220, 150);
    
    panels->registerCallback("Load Model", [this]() { 
        if (core) core->openFileDialogAndLoadModel(getHWND()); 
    });
    panels->registerCallback("Start", [this]() { 
        if (core) SwapFlag(*core); 
    });
    panels->registerCallback("Stop", [this]() { 
        if (core) SwapFlag(*core); 
    });
    panels->registerCallback("Create Empty", []() { 
        std::cout << "Create Empty" << std::endl; 
    });
    panels->registerCallback("Create Cube", []() { 
        std::cout << "Create Cube" << std::endl; 
    });
    panels->registerCallback("Create Sphere", []() { 
        std::cout << "Create Sphere" << std::endl; 
    });
    panels->registerCallback("Apply", []() { 
        std::cout << "Apply" << std::endl; 
    });
    panels->registerCallback("Reset", []() { 
        std::cout << "Reset" << std::endl; 
    });
    panels->registerCallback("Wireframe", []() { 
        std::cout << "Wireframe" << std::endl; 
    });
    panels->registerCallback("Solid", []() { 
        std::cout << "Solid" << std::endl; 
    });
    panels->registerCallback("Clear", []() { 
        std::cout << "Clear" << std::endl; 
    });
    
    auto top = panels->getPanel("TopBar");
    if (top) {
        top->addButton("Load Model", 230, 8, 100, 24, 0.4f, 0.5f, 0.4f, [this]() { 
            if (core) core->openFileDialogAndLoadModel(getHWND()); 
        });
        top->addButton("Start", 340, 8, 60, 24, 0.4f, 0.5f, 0.4f, [this]() { 
            if (core) SwapFlag(*core); 
        });
        top->addButton("Stop", 410, 8, 60, 24, 0.5f, 0.4f, 0.4f, [this]() { 
            if (core) SwapFlag(*core); 
        });
    }
    
    auto hierarchy = panels->getPanel("Hierarchy");
    if (hierarchy) {
        hierarchy->addButton("Create Empty", 10, 35, 100, 24, 0.4f, 0.4f, 0.5f, []() { 
            std::cout << "Create Empty" << std::endl; 
        });
        hierarchy->addButton("Create Cube", 10, 65, 100, 24, 0.4f, 0.4f, 0.5f, []() { 
            std::cout << "Create Cube" << std::endl; 
        });
        hierarchy->addButton("Create Sphere", 10, 95, 100, 24, 0.4f, 0.4f, 0.5f, []() { 
            std::cout << "Create Sphere" << std::endl; 
        });
        hierarchy->addLabel("Objects: 0", 10, 130, 12, false, 0.7f, 0.7f, 0.7f);
        hierarchy->addLabel("Selected: None", 10, 150, 12, false, 0.7f, 0.7f, 0.7f);
    }
    
    auto inspector = panels->getPanel("Inspector");
    if (inspector) {
        inspector->addButton("Apply", 10, 35, 80, 24, 0.4f, 0.5f, 0.4f, []() { 
            std::cout << "Apply" << std::endl; 
        });
        inspector->addButton("Reset", 100, 35, 80, 24, 0.5f, 0.4f, 0.4f, []() { 
            std::cout << "Reset" << std::endl; 
        });
        inspector->addLabel("Position: 0,0,0", 10, 70, 12, false, 0.8f, 0.8f, 0.8f);
        inspector->addLabel("Rotation: 0,0,0", 10, 90, 12, false, 0.8f, 0.8f, 0.8f);
        inspector->addLabel("Scale: 1,1,1", 10, 110, 12, false, 0.8f, 0.8f, 0.8f);
    }
    
    auto view3D = panels->getPanel("3D Viewport");
    if (view3D) {
        view3D->addButton("Wireframe", 10, 35, 80, 24, 0.4f, 0.4f, 0.5f, []() { 
            std::cout << "Wireframe" << std::endl; 
        });
        view3D->addButton("Solid", 100, 35, 80, 24, 0.4f, 0.4f, 0.5f, []() { 
            std::cout << "Solid" << std::endl; 
        });
    }
    
    auto console = panels->getPanel("Console");
    if (console) {
        console->addButton("Clear", 10, 35, 60, 24, 0.4f, 0.4f, 0.5f, []() { 
            std::cout << "Clear" << std::endl; 
        });
        console->addLabel("> Ready", 10, 70, 12, false, 0.7f, 0.8f, 0.7f);
    }
    
    panels->loadLayout("Config/WindowSettings.json");
}

InterfaceManager::~InterfaceManager() { 
    panels->saveLayout("Config/WindowSettings.json");
    delete panels; 
}

void InterfaceManager::setVulkan(Vulkan* vk) {
    renderer.setVulkan(vk);
}

InterfaceManager::Dimensions InterfaceManager::getDimensions() {
    Dimensions d = {0, 0};
    if (!window) return d;
    HWND hwnd = window->getHWND();
    if (!hwnd) return d;
    RECT rect;
    GetClientRect(hwnd, &rect);
    d.width = rect.right - rect.left;
    d.height = rect.bottom - rect.top;
    return d;
}

void InterfaceManager::clearScreen(int width, int height) {
    if (currentAPI == RenderAPI::OPENGL) {
        glViewport(0, 0, width, height);
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
}

void InterfaceManager::setup3DViewport(const Dimensions& dims) {
    Panel* view3D = panels ? panels->get3D() : nullptr;
    
    if (currentAPI == RenderAPI::OPENGL) {
        if (view3D && view3D->visible) {
            glViewport(view3D->getX(), dims.height - (view3D->getY() + view3D->getH()), 
                       view3D->getW(), view3D->getH());
        } else {
            glViewport(0, 0, dims.width, dims.height);
        }
        glEnable(GL_DEPTH_TEST);
    }
}

void InterfaceManager::renderStatic() {
    Dimensions d = getDimensions();
    if (d.width == 0 || d.height == 0) return;
    
    if (currentAPI == RenderAPI::OPENGL) {
        GLint prog, vp[4];
        GLboolean dt;
        renderer.saveState(prog, vp, dt);
        renderer.setup2D(d.width, d.height);
        
        panels->update(d.width, d.height);
        panels->render(renderer);
        objectUI.render(renderer, d.width, d.height, *panels);
        
        renderer.drawText(10, d.height - 25, "3D Viewer", 1.0f, 1.0f, 1.0f);
        if (core && core->modelLoaded) {
            std::string s = "Model: " + core->modelPath.substr(core->modelPath.find_last_of("/\\") + 1);
            renderer.drawText(10, d.height - 40, s, 0.5f, 0.8f, 0.5f);
        } else {
            renderer.drawText(10, d.height - 40, "No model loaded", 1.0f, 0.8f, 0.3f);
        }
        
        renderer.restoreMatrices();
        renderer.restoreState(prog, vp, dt);
    } else {
        // Для Vulkan - только вызовы draw, без beginFrame/endFrame
        renderer.setup2D(d.width, d.height);
        
        panels->update(d.width, d.height);
        panels->render(renderer);
        objectUI.render(renderer, d.width, d.height, *panels);
        
        renderer.drawText(10, d.height - 25, "3D Viewer (Vulkan)", 1.0f, 1.0f, 1.0f);
        if (core && core->modelLoaded) {
            std::string s = "Model: " + core->modelPath.substr(core->modelPath.find_last_of("/\\") + 1);
            renderer.drawText(10, d.height - 40, s, 0.5f, 0.8f, 0.5f);
        } else {
            renderer.drawText(10, d.height - 40, "No model loaded", 1.0f, 0.8f, 0.3f);
        }
    }
}

void InterfaceManager::renderDynamic() {
    Dimensions d = getDimensions();
    if (d.width == 0 || d.height == 0) return;
    
    if (currentAPI == RenderAPI::OPENGL) {
        GLint prog, vp[4];
        GLboolean dt;
        renderer.saveState(prog, vp, dt);
        renderer.setup2D(d.width, d.height);
        renderer.restoreMatrices();
        renderer.restoreState(prog, vp, dt);
    }
}

void InterfaceManager::handleClick(int x, int y) {
    isClick = true;
}

void InterfaceManager::handleMouseDown(int x, int y) { 
    panels->onMouseDown(x, y); 
}

void InterfaceManager::handleMouseMove(int x, int y) { 
    panels->onMouseMove(x, y);
    isClick = false;
    
    for (auto p : panels->getAll()) {
        if (!p->visible) continue;
        int edge = p->getEdge(x, y, 10);
        if (edge == 0 || edge == 1) {
            SetCursor(LoadCursor(NULL, IDC_SIZEWE));
            return;
        } else if (edge == 2 || edge == 3) {
            SetCursor(LoadCursor(NULL, IDC_SIZENS));
            return;
        }
    }
    SetCursor(LoadCursor(NULL, IDC_ARROW));
}

void InterfaceManager::handleMouseUp(int x, int y) { 
    panels->onMouseUp(x, y); 
}

void InterfaceManager::SwapFlag(Core &A) { 
    A.isStart = !A.isStart; 
}

HWND InterfaceManager::getHWND() const { 
    return window ? window->getHWND() : nullptr; 
}

void InterfaceManager::BlockMoveToMainWindow(int x, int y) {}

bool InterfaceManager::CheckerClickToPanel(int x, int y) { 
    return panels->at(x, y) != nullptr; 
}

bool InterfaceManager::isBlockingRender() {
    return false;
}

void InterfaceManager::setWindow(InitialWin32* win) {
    window = win;
}

void InterfaceManager::initializeRender(HWND hwnd, int width, int height) {
    renderer.initialize(hwnd, width, height);
    // Vulkan будет передан через setVulkan из Core
}

void InterfaceManager::updateWindowSize(int width, int height) {
    renderer.setup2D(width, height);
}