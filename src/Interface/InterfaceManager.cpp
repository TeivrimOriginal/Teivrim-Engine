#include "InterfaceManager.h"
#include <iostream>

InterfaceManager::InterfaceManager(Core* corePtr) : window(nullptr), core(corePtr) {
    objectUI.createButton("Button 1", 10, 10, 80, 25, []() {
        std::cout << "Button 1 clicked!" << std::endl;
    });
    
    objectUI.createButton("Button 2", 100, 10, 80, 25, [this]() {
        std::cout << "Button 2 clicked!" << std::endl;
        if (core) {
            SwapFlag(*core);
            std::cout << "Core.isStart = " << core->isStart << std::endl;
        }
    });
    
    objectUI.createButton("Left Button", 20, 50, 100, 30, []() {
        std::cout << "Left panel button clicked!" << std::endl;
    });
    
    objectUI.createButton("Right Button", 20, 100, 100, 30, []() {
        std::cout << "Right panel button clicked!" << std::endl;
    });
    
    objectUI.createButton("Load Model", 20, 5, 160, 30, [this]() {
        std::cout << "Load Model clicked" << std::endl;
        if (core) {
            HWND hwnd = getHWND();
            if (core->openFileDialogAndLoadModel(hwnd)) {
                std::cout << "Model successfully loaded" << std::endl;
            } else {
                std::cout << "Model not loaded or cancelled" << std::endl;
            }
        }
    });
    
    objectUI.attachToPanel("Button 1", PanelType::Top);
    objectUI.attachToPanel("Button 2", PanelType::Top);
    objectUI.attachToPanel("Left Button", PanelType::Left);
    objectUI.attachToPanel("Right Button", PanelType::Right);
    objectUI.attachToPanel("Load Model", PanelType::Bottom);
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
    PanelDimensions panelDims = panels.getDimensions(dims.width, dims.height);
    
    glViewport(panelDims.leftPanelWidth, panelDims.topPanelHeight,
               dims.width - (panelDims.leftPanelWidth + panelDims.rightPanelWidth),
               dims.height - (panelDims.topPanelHeight + panelDims.bottomPanelHeight));
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
    
    glBegin(GL_QUADS);
    panels.render(renderer, dims.width, dims.height);
    objectUI.render(renderer, dims.width, dims.height, panels);
    glEnd();
    
    renderer.drawText(10, dims.height - 25, "3D Viewer v1.0", 1.0f, 1.0f, 1.0f);
    
    if (core && core->modelLoaded) {
        std::string status = "Model: " + core->modelPath.substr(core->modelPath.find_last_of("/\\") + 1);
        renderer.drawText(10, dims.height - 40, status, 0.5f, 0.8f, 0.5f);
    } else {
        renderer.drawText(10, dims.height - 40, "No model loaded", 1.0f, 0.8f, 0.3f);
    }
    
    renderer.drawText(dims.width - 150, dims.height - 25, "Press ESC to exit", 0.7f, 0.7f, 0.7f);
    
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
    Dimensions dims = getDimensions();
    if (dims.width == 0 || dims.height == 0) return;
    
    objectUI.handleClick(x, y, dims.width, dims.height, panels);
}

void InterfaceManager::SwapFlag(Core &A) {
    A.isStart = !A.isStart;
}

HWND InterfaceManager::getHWND() const {
    return window ? window->getHWND() : nullptr;
}