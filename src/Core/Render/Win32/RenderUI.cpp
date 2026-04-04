#include "RenderUI.h"
#include "GraphicsApiSupport/RenderUI_OpenGL.h"
#include "GraphicsApiSupport/RenderUI_Vulkan.h"
#include <string>
#include <vector>
#include <cstring>
#include <cstdio>

RenderUI::RenderUI(RenderAPIType api) 
    : currentAPI(api)
    , glImpl(nullptr)
    , vkImpl(nullptr)
    , isOpenGL(api == RenderAPIType::OPENGL) 
{
    if (isOpenGL) {
        glImpl = new RenderUI_OpenGL();
    } else {
        vkImpl = new RenderUI_Vulkan();
    }
}

RenderUI::~RenderUI() {
    if (glImpl) delete glImpl;
    if (vkImpl) delete vkImpl;
}

bool RenderUI::initialize(HWND hwnd, int width, int height) {
    if (isOpenGL) {
        // OpenGL не требует отдельной инициализации
        return true;
    } else {
        return vkImpl->initialize(hwnd, width, height);
    }
}

void RenderUI::cleanup() {
    if (!isOpenGL && vkImpl) {
        vkImpl->cleanup();
    }
}

void RenderUI::beginFrame() {
    if (!isOpenGL && vkImpl) {
        vkImpl->beginFrame();
    }
}

void RenderUI::endFrame() {
    if (!isOpenGL && vkImpl) {
        vkImpl->endFrame();
    }
}

void RenderUI::present() {
    if (!isOpenGL && vkImpl) {
        vkImpl->present();
    }
}

void RenderUI::saveState(GLint& prog, GLint vp[4], GLboolean& dt) {
    if (isOpenGL && glImpl) {
        glImpl->saveState(prog, vp, dt);
    }
}

void RenderUI::restoreState(GLint prog, GLint vp[4], GLboolean dt) {
    if (isOpenGL && glImpl) {
        glImpl->restoreState(prog, vp, dt);
    }
}

void RenderUI::setup2D(int width, int height) {
    if (isOpenGL && glImpl) {
        glImpl->setup2D(width, height);
    } else if (vkImpl) {
        vkImpl->setup2D(width, height);
    }
}

void RenderUI::restoreMatrices() {
    if (isOpenGL && glImpl) {
        glImpl->restoreMatrices();
    }
}

void RenderUI::drawQuad(float x1, float y1, float x2, float y2, float r, float g, float b) {
    if (isOpenGL && glImpl) {
        glImpl->drawQuad(x1, y1, x2, y2, r, g, b);
    } else if (vkImpl) {
        vkImpl->drawQuad(x1, y1, x2, y2, r, g, b);
    }
}

void RenderUI::drawQuad(int x1, int y1, int x2, int y2, float r, float g, float b) {
    drawQuad((float)x1, (float)y1, (float)x2, (float)y2, r, g, b);
}

void RenderUI::drawText(int x, int y, const std::string& text, float r, float g, float b) {
    if (isOpenGL && glImpl) {
        glImpl->drawText(x, y, text, r, g, b);
    } else if (vkImpl) {
        vkImpl->drawText(x, y, text, r, g, b);
    }
}

void RenderUI::drawTextCentered(int x, int y, int w, int h, const std::string& text, float r, float g, float b) {
    if (isOpenGL && glImpl) {
        glImpl->drawTextCentered(x, y, w, h, text, r, g, b);
    } else if (vkImpl) {
        vkImpl->drawTextCentered(x, y, w, h, text, r, g, b);
    }
}

void RenderUI::drawPanel(float x1, float y1, float x2, float y2, float r, float g, float b) {
    drawQuad(x1, y1, x2, y2, r, g, b);
}