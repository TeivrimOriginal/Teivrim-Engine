#include "rendererv.h"
#include <iostream>

RendererV::RendererV() : window(nullptr), animateModel(true), initialized(false) {
    std::cout << "[VULKAN 3D] Constructor (STUB - no 3D rendering)" << std::endl;
}

RendererV::~RendererV() {
    cleanup();
}

bool RendererV::initialize(InitialWin32* win) {
    window = win;
    initialized = true;
    std::cout << "[VULKAN 3D] Initialized (STUB MODE - 3D models won't render)" << std::endl;
    return true;
}

void RendererV::cleanup() {
    initialized = false;
    std::cout << "[VULKAN 3D] Cleanup" << std::endl;
}

void RendererV::renderModel(Camera& camera) {
    // Заглушка - ничего не рендерит
    if (!initialized) return;
    // 3D модели через Vulkan НЕ РЕНДЕРЯТСЯ
}

void RendererV::setAnimateModel(bool animate) {
    animateModel = animate;
}