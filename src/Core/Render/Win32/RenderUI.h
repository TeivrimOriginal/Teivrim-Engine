#ifndef RENDERUI_H
#define RENDERUI_H

#include <GL/glew.h>
#include <windows.h>
#include <string>
#include <memory>

#include "GraphicsApiSupport/RenderUI_OpenGL.h"

class RenderUI {
public:
    RenderUI();
    ~RenderUI();
    
    void drawUI(HWND hwnd);
    void saveState(GLint& prog, GLint vp[4], GLboolean& dt);
    void restoreState(GLint prog, GLint vp[4], GLboolean dt);
    void setup2D(int width, int height);
    void restoreMatrices();
    
    void drawQuad(float x1, float y1, float x2, float y2, float r, float g, float b);
    void drawQuad(int x1, int y1, int x2, int y2, float r, float g, float b);
    
    void drawText(int x, int y, const std::string& text, float r, float g, float b);
    void drawTextCentered(int x, int y, int w, int h, const std::string& text, float r, float g, float b);

private:
    void drawPanel(float x1, float y1, float x2, float y2, float r, float g, float b);
    
    std::unique_ptr<RenderUI_OpenGL> renderImpl;
};

#endif