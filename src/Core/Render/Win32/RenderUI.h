#ifndef RENDERUI_H
#define RENDERUI_H

#include <GL/glew.h>
#include <windows.h>
#include <string>

enum class RenderAPIType {
    OPENGL,
    VULKAN
};

class Vulkan;  // forward declaration

class RenderUI {
public:
    RenderUI(RenderAPIType api = RenderAPIType::OPENGL);
    ~RenderUI();
    
    bool initialize(HWND hwnd, int width, int height);
    void cleanup();
    void setVulkan(Vulkan* vk);
    
    void beginFrame();
    void endFrame();
    void present();
    
    void saveState(GLint& prog, GLint vp[4], GLboolean& dt);
    void restoreState(GLint prog, GLint vp[4], GLboolean dt);
    void setup2D(int width, int height);
    void restoreMatrices();
    
    void drawQuad(float x1, float y1, float x2, float y2, float r, float g, float b);
    void drawQuad(int x1, int y1, int x2, int y2, float r, float g, float b);
    
    void drawText(int x, int y, const std::string& text, float r, float g, float b);
    void drawTextCentered(int x, int y, int w, int h, const std::string& text, float r, float g, float b);
    
    RenderAPIType getCurrentAPI() const { return currentAPI; }

private:
    RenderAPIType currentAPI;
    class RenderUI_OpenGL* glImpl;
    class Vulkan* vkImpl;  // теперь указатель на Vulkan класс
    bool isOpenGL;
};

#endif