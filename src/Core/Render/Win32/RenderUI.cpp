#include "RenderUI.h"
#include "GraphicsApiSupport/RenderUI_OpenGL.h"
#include "../../Vulkan.h"
#include <iostream>

RenderUI::RenderUI(RenderAPIType api) 
    : currentAPI(api), isOpenGL(api == RenderAPIType::OPENGL), glImpl(nullptr), vkImpl(nullptr)
{
    if (isOpenGL) {
        glImpl = new RenderUI_OpenGL();
        std::cout << "RenderUI: OpenGL mode" << std::endl;
    } else {
        // vkImpl будет установлен через setVulkan()
        std::cout << "RenderUI: Vulkan mode (waiting for Vulkan instance)" << std::endl;
    }
}

RenderUI::~RenderUI() {
    delete glImpl;
    // vkImpl не удаляем - он принадлежит Core
}

bool RenderUI::initialize(HWND hwnd, int width, int height) {
    if (isOpenGL && glImpl) return true;
    if (vkImpl) return true;  // уже установлен
    return false;
}

void RenderUI::setVulkan(Vulkan* vk) {
    vkImpl = vk;
    std::cout << "RenderUI: Vulkan instance set" << std::endl;
}

void RenderUI::cleanup() {
    // ничего не делаем
}

void RenderUI::beginFrame() {
    if (vkImpl) vkImpl->beginFrame();
}

void RenderUI::endFrame() {
    if (vkImpl) vkImpl->endFrame();
}

void RenderUI::present() {
    if (vkImpl) vkImpl->present();
}

void RenderUI::saveState(GLint& prog, GLint vp[4], GLboolean& dt) {
    if (glImpl) glImpl->saveState(prog, vp, dt);
}

void RenderUI::restoreState(GLint prog, GLint vp[4], GLboolean dt) {
    if (glImpl) glImpl->restoreState(prog, vp, dt);
}

void RenderUI::setup2D(int width, int height) {
    if (glImpl) glImpl->setup2D(width, height);
    if (vkImpl) vkImpl->setup2D(width, height);
}

void RenderUI::restoreMatrices() {
    if (glImpl) glImpl->restoreMatrices();
}

void RenderUI::drawQuad(float x1, float y1, float x2, float y2, float r, float g, float b) {
    if (glImpl) glImpl->drawQuad(x1, y1, x2, y2, r, g, b);
    if (vkImpl) vkImpl->drawQuad(x1, y1, x2, y2, r, g, b);
}

void RenderUI::drawQuad(int x1, int y1, int x2, int y2, float r, float g, float b) {
    drawQuad((float)x1, (float)y1, (float)x2, (float)y2, r, g, b);
}

void RenderUI::drawText(int x, int y, const std::string& text, float r, float g, float b) {
    if (glImpl) glImpl->drawText(x, y, text, r, g, b);
    if (vkImpl) vkImpl->drawText(x, y, text, r, g, b);
}

void RenderUI::drawTextCentered(int x, int y, int w, int h, const std::string& text, float r, float g, float b) {
    if (glImpl) glImpl->drawTextCentered(x, y, w, h, text, r, g, b);
    if (vkImpl) vkImpl->drawTextCentered(x, y, w, h, text, r, g, b);
}