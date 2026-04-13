#include "RenderUI.h"
#include <iostream>

RenderUI::RenderUI(RenderAPIType api) 
    : currentAPI(api), 
      isOpenGL(api == RenderAPIType::OPENGL), 
      vkImpl(nullptr)
{
    if (isOpenGL) {
        std::cout << "RenderUI: OpenGL mode (disabled)" << std::endl;
    } else {
        std::cout << "RenderUI: Vulkan mode (stub)" << std::endl;
    }
}

RenderUI::~RenderUI() {
}

bool RenderUI::initialize(HWND hwnd, int width, int height) {
    std::cout << "RenderUI::initialize() called" << std::endl;
    return true;
}

void RenderUI::setVulkan(void* vk) {
    vkImpl = vk;
    std::cout << "RenderUI: Vulkan instance set" << std::endl;
}

void RenderUI::cleanup() {
}

void RenderUI::beginFrame() {
}

void RenderUI::endFrame() {
}

void RenderUI::present() {
}

void RenderUI::saveState(int& prog, int vp[4], bool& dt) {
}

void RenderUI::restoreState(int prog, int vp[4], bool dt) {
}

void RenderUI::setup2D(int width, int height) {
}

void RenderUI::restoreMatrices() {
}

void RenderUI::drawQuad(float x1, float y1, float x2, float y2, float r, float g, float b) {
}

void RenderUI::drawQuad(int x1, int y1, int x2, int y2, float r, float g, float b) {
    drawQuad((float)x1, (float)y1, (float)x2, (float)y2, r, g, b);
}

void RenderUI::drawText(int x, int y, const std::string& text, float r, float g, float b) {
}

void RenderUI::drawTextCentered(int x, int y, int w, int h, const std::string& text, float r, float g, float b) {
}