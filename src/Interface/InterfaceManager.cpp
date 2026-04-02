#include "InterfaceManager.h"
#include <iostream>

InterfaceManager::InterfaceManager(Core* corePtr) : window(nullptr), core(corePtr) {
    panels = new PanelManager();
    
    Dimensions dims = getDimensions();
    int sw = dims.width > 0 ? dims.width : 1280;
    int sh = dims.height > 0 ? dims.height : 720;
    
    auto top = panels->add("Scene Control", 0, 0, sw, 50);
    top->setDock(2, sw, sh);
    top->addButton("Load Model", [this]() { if (core) core->openFileDialogAndLoadModel(getHWND()); });
    top->addButton("Start", [this]() { if (core) SwapFlag(*core); });
    top->addButton("Stop", [this]() { if (core) SwapFlag(*core); });
    top->addLabel("FPS: 0");
    
    auto bottom = panels->add("Asset Manager", 0, sh - 100, sw, 100);
    bottom->setDock(3, sw, sh);
    bottom->addButton("Import", []() { std::cout << "Import" << std::endl; });
    bottom->addButton("Export", []() { std::cout << "Export" << std::endl; });
    bottom->addLabel("Ready");
    
    auto left = panels->add("Hierarchy", 0, 50, 250, sh - 150);
    left->setDock(0, sw, sh);
    left->addButton("Create Empty", []() { std::cout << "Create Empty" << std::endl; });
    left->addButton("Create Cube", []() { std::cout << "Create Cube" << std::endl; });
    left->addButton("Create Sphere", []() { std::cout << "Create Sphere" << std::endl; });
    left->addLabel("Objects: 0");
    left->addLabel("Selected: None");
    
    auto right = panels->add("Inspector", sw - 260, 50, 260, sh - 150);
    right->setDock(1, sw, sh);
    right->addButton("Apply", []() { std::cout << "Apply" << std::endl; });
    right->addButton("Reset", []() { std::cout << "Reset" << std::endl; });
    right->addLabel("Position: 0,0,0");
    right->addLabel("Rotation: 0,0,0");
    right->addLabel("Scale: 1,1,1");
    
    auto view3D = panels->add("3D Viewport", 250, 50, sw - 510, sh - 150, true);
    view3D->setMinSize(300, 200);
    view3D->addButton("Wireframe", []() { std::cout << "Wireframe" << std::endl; });
    view3D->addButton("Solid", []() { std::cout << "Solid" << std::endl; });
}

InterfaceManager::~InterfaceManager() { 
    delete panels; 
}

Dimensions InterfaceManager::getDimensions() {
    Dimensions dims = {0, 0};
    if (!window) return dims;
    HWND hwnd = window->getHWND();
    if (!hwnd) return dims;
    RECT rect;
    GetClientRect(hwnd, &rect);
    dims.width = rect.right - rect.left;
    dims.height = rect.bottom - rect.top;
    return dims;
}

void InterfaceManager::clearScreen(int width, int height) {
    glViewport(0, 0, width, height);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void InterfaceManager::setup3DViewport(const Dimensions& dims) {
    Panel* view3D = panels ? panels->get3D() : nullptr;
    if (view3D && view3D->isVisible()) {
        glViewport(view3D->getX(), dims.height - (view3D->getY() + view3D->getH()), 
                   view3D->getW(), view3D->getH());
    } else {
        glViewport(0, 0, dims.width, dims.height);
    }
    glEnable(GL_DEPTH_TEST);
}

void InterfaceManager::renderStatic() {
    Dimensions dims = getDimensions();
    if (dims.width == 0 || dims.height == 0) return;
    
    GLint program;
    GLint viewport[4];
    GLboolean depthTest;
    renderer.saveState(program, viewport, depthTest);
    
    renderer.setup2D(dims.width, dims.height);
    
    if (panels) {
        panels->updateDocks(dims.width, dims.height);
        panels->render(renderer);
    }
    
    objectUI.render(renderer, dims.width, dims.height, *panels);
    
    renderer.drawText(10, dims.height - 25, "3D Viewer", 1.0f, 1.0f, 1.0f);
    
    if (core && core->modelLoaded) {
        std::string status = "Model: " + core->modelPath.substr(core->modelPath.find_last_of("/\\") + 1);
        renderer.drawText(10, dims.height - 40, status, 0.5f, 0.8f, 0.5f);
    } else {
        renderer.drawText(10, dims.height - 40, "No model loaded", 1.0f, 0.8f, 0.3f);
    }
    
    renderer.drawText(dims.width - 150, dims.height - 25, "ESC to exit", 0.7f, 0.7f, 0.7f);
    
    renderer.restoreMatrices();
    renderer.restoreState(program, viewport, depthTest);
}

void InterfaceManager::renderDynamic() {
    Dimensions dims = getDimensions();
    if (dims.width == 0 || dims.height == 0) return;
    
    GLint program;
    GLint viewport[4];
    GLboolean depthTest;
    renderer.saveState(program, viewport, depthTest);
    
    renderer.setup2D(dims.width, dims.height);
    
    renderer.restoreMatrices();
    renderer.restoreState(program, viewport, depthTest);
}

void InterfaceManager::handleClick(int x, int y) {
    if (panels) {
        Panel* p = panels->at(x, y);
        if (p && p->closeClicked(x, y)) {
            p->setVisible(false);
            return;
        }
        if (p) p->handleClick(x, y);
    }
}

void InterfaceManager::handleMouseDown(int x, int y) {
    Dimensions dims = getDimensions();
    if (dims.width == 0 || dims.height == 0) return;
    
    if (panels) {
        panels->onMouseDown(x, y);
        if (panels->isDragging() && window) {
            SetCapture(window->getHWND());
        }
    }
}

void InterfaceManager::handleMouseMove(int x, int y) {
    Dimensions dims = getDimensions();
    if (dims.width == 0 || dims.height == 0) return;
    
    if (panels) {
        panels->onMouseMove(x, y);
        if (panels->isDragging()) return;
        
        Panel* p = panels->at(x, y);
        if (p && p->getEdge(x, y) != -1) {
            int e = p->getEdge(x, y);
            HCURSOR cursor;
            if (e == 0 || e == 1) cursor = LoadCursor(NULL, IDC_SIZEWE);
            else if (e == 2 || e == 3) cursor = LoadCursor(NULL, IDC_SIZENS);
            else cursor = LoadCursor(NULL, IDC_ARROW);
            SetCursor(cursor);
            return;
        }
    }
    SetCursor(LoadCursor(NULL, IDC_ARROW));
}

void InterfaceManager::handleMouseUp(int x, int y) {
    if (panels) {
        panels->onMouseUp(x, y);
    }
    if (window) {
        ReleaseCapture();
    }
}

void InterfaceManager::SwapFlag(Core &A) {
    A.isStart = !A.isStart;
}

HWND InterfaceManager::getHWND() const {
    return window ? window->getHWND() : nullptr;
}

void InterfaceManager::BlockMoveToMainWindow(int x, int y) {}

bool InterfaceManager::CheckerClickToPanel(int x, int y) {
    return panels ? panels->at(x, y) != nullptr : false;
}