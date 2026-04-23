#include "RenderUI_Vulkan.h"
#include "VkInit.h"
#include <iostream>

RenderUI_Vulkan::RenderUI_Vulkan() : windowWidth(0), windowHeight(0), hwnd(nullptr), initialized(false) {}

RenderUI_Vulkan::~RenderUI_Vulkan() {}

bool RenderUI_Vulkan::initialize(HWND hwnd, int width, int height) {
    this->hwnd = hwnd;
    this->windowWidth = width;
    this->windowHeight = height;
    initialized = true;
    printf("[VULKAN UI] Initialized\n");
    return true;
}

void RenderUI_Vulkan::cleanup() {
    initialized = false;
}

void RenderUI_Vulkan::beginFrame() {}
void RenderUI_Vulkan::endFrame() {
    VkInit::renderFrame();
}
void RenderUI_Vulkan::present() {}

void RenderUI_Vulkan::setup2D(int width, int height) {
    if (width > 0 && height > 0) {
        windowWidth = width;
        windowHeight = height;
    }
}

void RenderUI_Vulkan::drawQuad(float x1, float y1, float x2, float y2, float r, float g, float b) {
    if (!initialized || windowWidth <= 0) return;
    
    float nx1 = (x1 / windowWidth) * 2.0f - 1.0f;
    float ny1 = (y1 / windowHeight) * 2.0f - 1.0f;
    float nx2 = (x2 / windowWidth) * 2.0f - 1.0f;
    float ny2 = (y2 / windowHeight) * 2.0f - 1.0f;
    
    struct Vertex { float x, y; float r, g, b, a; };
    std::vector<Vertex> vertices = {
        {nx1, ny1, r, g, b, 1.0f},
        {nx2, ny1, r, g, b, 1.0f},
        {nx2, ny2, r, g, b, 1.0f},
        {nx1, ny1, r, g, b, 1.0f},
        {nx2, ny2, r, g, b, 1.0f},
        {nx1, ny2, r, g, b, 1.0f}
    };
    
    VkInit::setUIData(vertices.data(), vertices.size(), sizeof(Vertex));
}

void RenderUI_Vulkan::drawQuad(int x1, int y1, int x2, int y2, float r, float g, float b) {
    drawQuad((float)x1, (float)y1, (float)x2, (float)y2, r, g, b);
}

void RenderUI_Vulkan::drawText(int x, int y, const std::string& text, float r, float g, float b) {
    float w = text.length() * 8;
    float h = 16;
    drawQuad((float)x, (float)y, (float)(x + w), (float)(y + h), r, g, b);
}

void RenderUI_Vulkan::drawTextCentered(int x, int y, int w, int h, const std::string& text, float r, float g, float b) {
    float tw = text.length() * 8;
    float th = 16;
    drawQuad(x + (w - tw) / 2, y + (h - th) / 2, x + (w + tw) / 2, y + (h + th) / 2, r, g, b);
}