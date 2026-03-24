#include "RenderUI_OpenGL.h"
#include <cstring>
#include <cstdio>

RenderUI_OpenGL::RenderUI_OpenGL() 
    : fontTexture(0)
    , fontInitialized(false) 
{
}

RenderUI_OpenGL::~RenderUI_OpenGL() {
    if (fontTexture) {
        glDeleteTextures(1, &fontTexture);
    }
}

void RenderUI_OpenGL::saveState(GLint& prog, GLint vp[4], GLboolean& dt) {
    glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
    glGetIntegerv(GL_VIEWPORT, vp);
    glGetBooleanv(GL_DEPTH_TEST, &dt);
}

void RenderUI_OpenGL::restoreState(GLint prog, GLint vp[4], GLboolean dt) {
    glViewport(vp[0], vp[1], vp[2], vp[3]);
    if (dt) glEnable(GL_DEPTH_TEST);
    else glDisable(GL_DEPTH_TEST);
    glUseProgram(prog);
}

void RenderUI_OpenGL::setup2D(int width, int height) {
    glViewport(0, 0, width, height);
    glUseProgram(0);
    glDisable(GL_DEPTH_TEST);
    
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, width, height, 0, -1, 1);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
}

void RenderUI_OpenGL::restoreMatrices() {
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void RenderUI_OpenGL::drawQuad(float x1, float y1, float x2, float y2, float r, float g, float b) {
    glColor3f(r, g, b);
    glVertex2f(x1, y1);
    glVertex2f(x2, y1);
    glVertex2f(x2, y2);
    glVertex2f(x1, y2);
}

void RenderUI_OpenGL::drawQuad(int x1, int y1, int x2, int y2, float r, float g, float b) {
    drawQuad((float)x1, (float)y1, (float)x2, (float)y2, r, g, b);
}

void RenderUI_OpenGL::initFont() {
    if (fontInitialized) return;
    
    const char* fontPaths[] = {
        "fonts/arial.ttf",
        "fonts/tahoma.ttf",
        "./fonts/arial.ttf",
        "../fonts/arial.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/tahoma.ttf",
        "C:/Windows/Fonts/consola.ttf"
    };
    
    FILE* fontFile = nullptr;
    char currentDir[1024];
    GetCurrentDirectoryA(1024, currentDir);
    printf("Current directory: %s\n", currentDir);
    
    for (int i = 0; i < 7; i++) {
        printf("Trying: %s\n", fontPaths[i]);
        fontFile = fopen(fontPaths[i], "rb");
        if (fontFile) {
            printf("SUCCESS: Font loaded from: %s\n", fontPaths[i]);
            break;
        }
    }
    
    if (!fontFile) {
        printf("ERROR: Cannot load any font file\n");
        return;
    }
    
    fseek(fontFile, 0, SEEK_END);
    long size = ftell(fontFile);
    fseek(fontFile, 0, SEEK_SET);
    
    unsigned char* fontBuffer = new unsigned char[size];
    fread(fontBuffer, 1, size, fontFile);
    fclose(fontFile);
    
    const int atlasWidth = 512;
    const int atlasHeight = 512;
    unsigned char* atlasBitmap = new unsigned char[atlasWidth * atlasHeight];
    memset(atlasBitmap, 0, atlasWidth * atlasHeight);
    
    int result = stbtt_BakeFontBitmap(fontBuffer, 0, 16.0f, atlasBitmap, 
                                       atlasWidth, atlasHeight, 32, 96, glyphs);
    
    if (result <= 0) {
        printf("ERROR: Failed to bake font bitmap (result=%d)\n", result);
        delete[] fontBuffer;
        delete[] atlasBitmap;
        return;
    }
    
    printf("Font baked successfully, atlas size: %d bytes\n", result);
    
    unsigned char* rgbaBitmap = new unsigned char[atlasWidth * atlasHeight * 4];
    for (int i = 0; i < atlasWidth * atlasHeight; i++) {
        unsigned char alpha = atlasBitmap[i];
        rgbaBitmap[i * 4 + 0] = 255;
        rgbaBitmap[i * 4 + 1] = 255;
        rgbaBitmap[i * 4 + 2] = 255;
        rgbaBitmap[i * 4 + 3] = alpha;
    }
    
    glGenTextures(1, &fontTexture);
    glBindTexture(GL_TEXTURE_2D, fontTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, atlasWidth, atlasHeight, 0, 
                 GL_RGBA, GL_UNSIGNED_BYTE, rgbaBitmap);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    delete[] fontBuffer;
    delete[] atlasBitmap;
    delete[] rgbaBitmap;
    
    fontInitialized = true;
    printf("Font texture created successfully (ID: %d)\n", fontTexture);
}

void RenderUI_OpenGL::drawText(int x, int y, const std::string& text, float r, float g, float b) {
    if (!fontInitialized) {
        initFont();
        if (!fontInitialized) return;
    }
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, fontTexture);
    
    float curX = (float)x;
    float curY = (float)y;
    
    glBegin(GL_QUADS);
    for (char c : text) {
        if (c < 32 || c > 127) continue;
        
        stbtt_bakedchar& ch = glyphs[c - 32];
        
        if (ch.x0 == 0 && ch.x1 == 0 && ch.y0 == 0 && ch.y1 == 0) {
            curX += ch.xadvance;
            continue;
        }
        
        float x0 = curX + ch.xoff;
        float y0 = curY + ch.yoff;
        float x1 = x0 + (ch.x1 - ch.x0);
        float y1 = y0 + (ch.y1 - ch.y0);
        
        float u0 = ch.x0 / 512.0f;
        float v0 = ch.y0 / 512.0f;
        float u1 = ch.x1 / 512.0f;
        float v1 = ch.y1 / 512.0f;
        
        glColor4f(r, g, b, 1.0f);
        glTexCoord2f(u0, v0); glVertex2f(x0, y0);
        glTexCoord2f(u1, v0); glVertex2f(x1, y0);
        glTexCoord2f(u1, v1); glVertex2f(x1, y1);
        glTexCoord2f(u0, v1); glVertex2f(x0, y1);
        
        curX += ch.xadvance;
    }
    glEnd();
    
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
}

void RenderUI_OpenGL::drawTextCentered(int x, int y, int w, int h, const std::string& text, float r, float g, float b) {
    if (!fontInitialized) {
        initFont();
        if (!fontInitialized) return;
    }
    
    float textWidth = 0;
    for (char c : text) {
        if (c >= 32 && c <= 127) {
            textWidth += glyphs[c - 32].xadvance;
        }
    }
    
    float textHeight = 16.0f;
    
    int centerX = x + (int)((w - textWidth) / 2);
    int centerY = y + (int)((h - textHeight) / 2);
    
    drawText(centerX, centerY, text, r, g, b);
}