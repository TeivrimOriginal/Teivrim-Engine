#include "RenderUI.h"
#include <string>
#include <vector>
#include <cstring>
#include <cstdio>

RenderUI::RenderUI() 
    : renderImpl(std::make_unique<RenderUI_OpenGL>())
{
}

RenderUI::~RenderUI() {
}

void RenderUI::saveState(GLint& prog, GLint vp[4], GLboolean& dt) {
    renderImpl->saveState(prog, vp, dt);
}

void RenderUI::restoreState(GLint prog, GLint vp[4], GLboolean dt) {
    renderImpl->restoreState(prog, vp, dt);
}

void RenderUI::setup2D(int width, int height) {
    renderImpl->setup2D(width, height);
}

void RenderUI::restoreMatrices() {
    renderImpl->restoreMatrices();
}

void RenderUI::drawQuad(float x1, float y1, float x2, float y2, float r, float g, float b) {
    renderImpl->drawQuad(x1, y1, x2, y2, r, g, b);
}

void RenderUI::drawQuad(int x1, int y1, int x2, int y2, float r, float g, float b) {
    renderImpl->drawQuad(x1, y1, x2, y2, r, g, b);
}

void RenderUI::drawText(int x, int y, const std::string& text, float r, float g, float b) {
    renderImpl->drawText(x, y, text, r, g, b);
}

void RenderUI::drawTextCentered(int x, int y, int w, int h, const std::string& text, float r, float g, float b) {
    renderImpl->drawTextCentered(x, y, w, h, text, r, g, b);
}

void RenderUI::drawPanel(float x1, float y1, float x2, float y2, float r, float g, float b) {
    drawQuad(x1, y1, x2, y2, r, g, b);
}

void RenderUI::drawUI(HWND hwnd) {
    if (!hwnd) return;
    
    RECT rect;
    GetClientRect(hwnd, &rect);
    int w = rect.right - rect.left;
    int h = rect.bottom - rect.top;
    
    if (w == 0 || h == 0) return;
    
    GLint prog, vp[4];
    GLboolean dt;
    saveState(prog, vp, dt);
    
    setup2D(w, h);
    
    glBegin(GL_QUADS);
    
    int panelW = (int)(w * 0.156f);
    int panelH = (int)(h * 0.046f);
    
    drawPanel(0, 0, panelW, h, 0.2f, 0.2f, 0.2f);
    drawPanel(w - panelW, 0, w, h, 0.2f, 0.2f, 0.2f);
    drawPanel(panelW, 0, w - panelW, panelH, 0.3f, 0.3f, 0.3f);
    drawPanel(panelW, h - panelH, w - panelW, h, 0.3f, 0.3f, 0.3f);
    
    glEnd();
    
    restoreMatrices();
    restoreState(prog, vp, dt);
}