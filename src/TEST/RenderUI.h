#ifndef RENDERUI_H
#define RENDERUI_H

#include <windows.h>
#include <string>

enum class RenderAPIType {
    OPENGL,
    VULKAN
};

enum class RenderAPI {
    OPENGL,
    VULKAN
};

class RenderUI {
public:
    RenderUI(RenderAPIType api = RenderAPIType::VULKAN);
    ~RenderUI();
    
    bool initialize(HWND hwnd, int width, int height);
    void cleanup();
    void setVulkan(void* vk);
    
    void beginFrame();
    void endFrame();
    void present();
    
    void saveState(int& prog, int vp[4], bool& dt);
    void restoreState(int prog, int vp[4], bool dt);
    void setup2D(int width, int height);
    void restoreMatrices();
    
    void drawQuad(float x1, float y1, float x2, float y2, float r, float g, float b);
    void drawQuad(int x1, int y1, int x2, int y2, float r, float g, float b);
    
    void drawText(int x, int y, const std::string& text, float r, float g, float b);
    void drawTextCentered(int x, int y, int w, int h, const std::string& text, float r, float g, float b);
    
    RenderAPIType getCurrentAPI() const { return currentAPI; }

private:
    RenderAPIType currentAPI;
    void* vkImpl;
    bool isOpenGL;
};

#endif