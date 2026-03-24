#ifndef RENDERUI_OPENGL_H
#define RENDERUI_OPENGL_H

#include <GL/glew.h>
#include <windows.h>
#include <string>

#include "stb_truetype.h"

class RenderUI_OpenGL {
public:
    RenderUI_OpenGL();
    ~RenderUI_OpenGL();
    
    void saveState(GLint& prog, GLint vp[4], GLboolean& dt);
    void restoreState(GLint prog, GLint vp[4], GLboolean dt);
    void setup2D(int width, int height);
    void restoreMatrices();
    
    void drawQuad(float x1, float y1, float x2, float y2, float r, float g, float b);
    void drawQuad(int x1, int y1, int x2, int y2, float r, float g, float b);
    
    void drawText(int x, int y, const std::string& text, float r, float g, float b);
    void drawTextCentered(int x, int y, int w, int h, const std::string& text, float r, float g, float b);
    
private:
    void initFont();
    
    GLuint fontTexture;
    bool fontInitialized;
    stbtt_bakedchar glyphs[96];
};

#endif