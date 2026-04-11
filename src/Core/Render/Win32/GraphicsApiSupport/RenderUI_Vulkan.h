#ifndef RENDERUI_VULKAN_H
#define RENDERUI_VULKAN_H

#include <windows.h>
#include <string>
#include <vector>
#include "VkInit.h"

class RenderUI_Vulkan {
public:
    RenderUI_Vulkan();
    ~RenderUI_Vulkan();
    
    bool initialize(HWND hwnd, int width, int height);
    void cleanup();
    void beginFrame();
    void endFrame();
    void present();
    void setup2D(int width, int height);
    
    void drawQuad(float x1, float y1, float x2, float y2, float r, float g, float b);
    void drawQuad(int x1, int y1, int x2, int y2, float r, float g, float b);
    void drawText(int x, int y, const std::string& text, float r, float g, float b);
    void drawTextCentered(int x, int y, int w, int h, const std::string& text, float r, float g, float b);
    
private:
    int windowWidth;
    int windowHeight;
    HWND hwnd;
    bool initialized;
};

#endif