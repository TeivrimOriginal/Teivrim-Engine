#include "InterfaceManager.h"
#include <iostream>

InterfaceManager::InterfaceManager(Core* corePtr) : window(nullptr), core(corePtr) {
    panels = new PanelManager();
    
    int sw = 1280, sh = 720;
    
    panels->add("TopBar", 0, 0, sw, 40);
    panels->add("Hierarchy", 0, 40, 220, sh - 40);
    panels->add("Inspector", sw - 260, 40, 260, sh - 40);
    panels->add("3D Viewport", 220, 40, sw - 480, sh - 40, true);
    panels->add("Console", 220, sh - 150, sw - 220, 150);
    
    panels->registerCallback("Load Model", [this]() { if (core) core->openFileDialogAndLoadModel(getHWND()); });
    panels->registerCallback("Start", [this]() { if (core) SwapFlag(*core); });
    panels->registerCallback("Stop", [this]() { if (core) SwapFlag(*core); });
    panels->registerCallback("Create Empty", []() { std::cout << "Create Empty" << std::endl; });
    panels->registerCallback("Create Cube", []() { std::cout << "Create Cube" << std::endl; });
    panels->registerCallback("Create Sphere", []() { std::cout << "Create Sphere" << std::endl; });
    panels->registerCallback("Apply", []() { std::cout << "Apply" << std::endl; });
    panels->registerCallback("Reset", []() { std::cout << "Reset" << std::endl; });
    panels->registerCallback("Wireframe", []() { std::cout << "Wireframe" << std::endl; });
    panels->registerCallback("Solid", []() { std::cout << "Solid" << std::endl; });
    panels->registerCallback("Clear", []() { std::cout << "Clear" << std::endl; });
    panels->registerCallback("File", []() { std::cout << "File" << std::endl; });
    panels->registerCallback("Edit", []() { std::cout << "Edit" << std::endl; });
    panels->registerCallback("View", []() { std::cout << "View" << std::endl; });
    
    panels->loadConfig("Config/UIElements.json");
    panels->loadLayout("Config/WindowSettings.json");
}

InterfaceManager::~InterfaceManager() { 
    panels->saveLayout("Config/WindowSettings.json");
    delete panels; 
}

Dimensions InterfaceManager::getDimensions() {
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
    glViewport(0, 0, width, height);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void InterfaceManager::setup3DViewport(const Dimensions& dims) {
    Panel* view3D = panels ? panels->get3D() : nullptr;
    if (view3D && view3D->visible) {
        glViewport(view3D->getX(), dims.height - (view3D->getY() + view3D->getH()), 
                   view3D->getW(), view3D->getH());
    } else {
        glViewport(0, 0, dims.width, dims.height);
    }
    glEnable(GL_DEPTH_TEST);
}

void InterfaceManager::renderStatic() {
    Dimensions d = getDimensions();
    if (d.width == 0 || d.height == 0) return;
    
    GLint prog, vp[4];
    GLboolean dt;
    renderer.saveState(prog, vp, dt);
    renderer.setup2D(d.width, d.height);
    
    panels->update(d.width, d.height);
    panels->render(renderer);
    objectUI.render(renderer, d.width, d.height, *panels);
    
    // panels->renderDebug(renderer);  // ЗАКОММЕНТИРОВАТЬ
    
    renderer.drawText(10, d.height - 25, "3D Viewer", 1.0f, 1.0f, 1.0f);
    if (core && core->modelLoaded) {
        std::string s = "Model: " + core->modelPath.substr(core->modelPath.find_last_of("/\\") + 1);
        renderer.drawText(10, d.height - 40, s, 0.5f, 0.8f, 0.5f);
    } else {
        renderer.drawText(10, d.height - 40, "No model loaded", 1.0f, 0.8f, 0.3f);
    }
    
    renderer.restoreMatrices();
    renderer.restoreState(prog, vp, dt);
}

void InterfaceManager::renderDynamic() {
    Dimensions d = getDimensions();
    if (d.width == 0 || d.height == 0) return;
    GLint prog, vp[4];
    GLboolean dt;
    renderer.saveState(prog, vp, dt);
    renderer.setup2D(d.width, d.height);
    renderer.restoreMatrices();
    renderer.restoreState(prog, vp, dt);
}

void InterfaceManager::handleClick(int x, int y) {}

void InterfaceManager::handleMouseDown(int x, int y) { 
    panels->onMouseDown(x, y); 
}

void InterfaceManager::handleMouseMove(int x, int y) { 
    panels->onMouseMove(x, y);
    
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

void InterfaceManager::SwapFlag(Core &A) { A.isStart = !A.isStart; }
HWND InterfaceManager::getHWND() const { return window ? window->getHWND() : nullptr; }
void InterfaceManager::BlockMoveToMainWindow(int x, int y) {}
bool InterfaceManager::CheckerClickToPanel(int x, int y) { return panels->at(x, y) != nullptr; }