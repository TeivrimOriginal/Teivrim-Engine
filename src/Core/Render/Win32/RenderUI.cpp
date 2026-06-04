#include "RenderUI.h"
#include "../Vulkan.h"

RenderUI::RenderUI(RenderAPIType type) : apiType(type), vulkan(nullptr), screenWidth(0), screenHeight(0), shaderProgram(0), vao(0), vbo(0) {}

RenderUI::~RenderUI() {}

void RenderUI::setVulkan(Vulkan* vk) { vulkan = vk; }

void RenderUI::initialize(HWND hwnd, int width, int height) {
    screenWidth = width;
    screenHeight = height;
    
    if (apiType == RenderAPIType::VULKAN && vulkan) {
        vulkan->setup2D(width, height);
    }
}

void RenderUI::setup2D(int width, int height) {
    screenWidth = width;
    screenHeight = height;
    
    if (apiType == RenderAPIType::VULKAN && vulkan) {
        vulkan->setup2D(width, height);
    }
}
void RenderUI::drawQuad(float x1, float y1, float x2, float y2, float r, float g, float b) {
    if (apiType == RenderAPIType::VULKAN && vulkan) {
        vulkan->drawQuad(x1, y1, x2, y2, r, g, b);
    }
}

void RenderUI::drawText(int x, int y, const std::string& text, float r, float g, float b) {
    if (apiType == RenderAPIType::VULKAN && vulkan) {
        vulkan->drawText(x, y, text, r, g, b);
    }
}

void RenderUI::drawTextCentered(int x, int y, int w, int h, const std::string& text, float r, float g, float b) {
    if (apiType == RenderAPIType::VULKAN && vulkan) {
        vulkan->drawTextCentered(x, y, w, h, text, r, g, b);
    }
}

void RenderUI::drawImage(int x, int y, int w, int h, void* texture) {
    if (apiType == RenderAPIType::VULKAN && vulkan && texture) {
        VulkanTexture* tex = (VulkanTexture*)texture;
        vulkan->drawImage(x, y, x + w, y + h, tex);
    }
}

GLuint RenderUI::loadTextureFromFile(const std::string& path) { return 0; }

void RenderUI::saveState(GLint& prog, GLint vp[4], GLboolean& dt) {}

void RenderUI::restoreState(GLint prog, GLint vp[4], GLboolean dt) {}

void RenderUI::restoreMatrices() {}