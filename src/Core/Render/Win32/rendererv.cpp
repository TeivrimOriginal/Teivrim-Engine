#include "rendererv.h"
#include "GraphicsApiSupport/VkInit.h"
#include <iostream>

RendererV::RendererV() : window(nullptr), animateModel(true), initialized(false) {
    std::cout << "[VULKAN 3D] Constructor" << std::endl;
}

RendererV::~RendererV() { cleanup(); }

bool RendererV::initialize(InitialWin32* win) {
    window = win;
    if (!window || !window->getHWND()) return false;
    
    RECT rect;
    GetClientRect(window->getHWND(), &rect);
    
    if (!VkInit::initialize(window->getHWND(), rect.right - rect.left, rect.bottom - rect.top))
        return false;
    
    initialized = true;
    std::cout << "[VULKAN 3D] Initialized" << std::endl;
    return true;
}

void RendererV::cleanup() {
    VkInit::cleanup();
    initialized = false;
}

void RendererV::renderModel(Camera& camera) {
    if (!initialized) return;
    
    struct Vertex { float x, y; float r, g, b, a; };
    Vertex vertices[] = {
        {0.0f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f},
        {0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 1.0f},
        {-0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f}
    };
    
    VkInit::set3DData(vertices, 3, sizeof(Vertex));
}

void RendererV::setAnimateModel(bool animate) {
    animateModel = animate;
}