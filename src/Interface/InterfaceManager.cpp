#include "InterfaceManager.h"
#include <iostream>

InterfaceManager::InterfaceManager(Core* corePtr) : window(nullptr), core(corePtr) {
    panels = new PanelManager();
    
    int sw = 1280, sh = 720;
    
    auto top = panels->add("TopBar", 0, 0, sw, 30);
    top->addButton("File", []() { std::cout << "File" << std::endl; });
    top->addButton("Edit", []() { std::cout << "Edit" << std::endl; });
    top->addButton("View", []() { std::cout << "View" << std::endl; });
    top->addButton("Load Model", [this]() { if (core) core->openFileDialogAndLoadModel(getHWND()); });
    top->addButton("Start", [this]() { if (core) SwapFlag(*core); });
    top->addButton("Stop", [this]() { if (core) SwapFlag(*core); });
    
    auto left = panels->add("Hierarchy", 0, 30, 220, sh - 30);
    left->addButton("Create Empty", []() { std::cout << "Create Empty" << std::endl; });
    left->addButton("Create Cube", []() { std::cout << "Create Cube" << std::endl; });
    left->addButton("Create Sphere", []() { std::cout << "Create Sphere" << std::endl; });
    left->addLabel("Objects: 0");
    
    auto right = panels->add("Inspector", sw - 260, 30, 260, sh - 30);
    right->addButton("Apply", []() { std::cout << "Apply" << std::endl; });
    right->addButton("Reset", []() { std::cout << "Reset" << std::endl; });
    right->addLabel("Position: 0,0,0");
    right->addLabel("Rotation: 0,0,0");
    right->addLabel("Scale: 1,1,1");
    
    auto view3D = panels->add("3D Viewport", 220, 30, sw - 480, sh - 30, true);
    view3D->addButton("Wireframe", []() { std::cout << "Wireframe" << std::endl; });
    view3D->addButton("Solid", []() { std::cout << "Solid" << std::endl; });
    
    auto console = panels->add("Console", 220, sh - 150, sw - 220, 150);
    console->addButton("Clear", []() { std::cout << "Clear" << std::endl; });
    console->addLabel("> Ready");
}

InterfaceManager::~InterfaceManager() { delete panels; }

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
void InterfaceManager::handleMouseDown(int x, int y) { panels->onMouseDown(x, y); if (panels->isDragging() && window) SetCapture(window->getHWND()); }
void InterfaceManager::handleMouseMove(int x, int y) { panels->onMouseMove(x, y); }
void InterfaceManager::handleMouseUp(int x, int y) { panels->onMouseUp(x, y); if (window) ReleaseCapture(); }
void InterfaceManager::SwapFlag(Core &A) { A.isStart = !A.isStart; }
HWND InterfaceManager::getHWND() const { return window ? window->getHWND() : nullptr; }
void InterfaceManager::BlockMoveToMainWindow(int x, int y) {}
bool InterfaceManager::CheckerClickToPanel(int x, int y) { return panels->at(x, y) != nullptr; }