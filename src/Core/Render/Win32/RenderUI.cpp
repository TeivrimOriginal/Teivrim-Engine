#include "RenderUI.h"

RenderUI::RenderUI() {}
RenderUI::~RenderUI() {}

void RenderUI::saveState(GLint& prog, GLint vp[4], GLboolean& dt) {
    glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
    glGetIntegerv(GL_VIEWPORT, vp);
    glGetBooleanv(GL_DEPTH_TEST, &dt);
}

void RenderUI::restoreState(GLint prog, GLint vp[4], GLboolean dt) {
    glViewport(vp[0], vp[1], vp[2], vp[3]);
    dt ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
    glUseProgram(prog);
}

void RenderUI::setup2D(int width, int height) {
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

void RenderUI::restoreMatrices() {
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void RenderUI::drawQuad(float x1, float y1, float x2, float y2, float r, float g, float b) {
    glColor3f(r, g, b);
    glVertex2f(x1, y1);
    glVertex2f(x2, y1);
    glVertex2f(x2, y2);
    glVertex2f(x1, y2);
}

void RenderUI::drawQuad(int x1, int y1, int x2, int y2, float r, float g, float b) {
    drawQuad((float)x1, (float)y1, (float)x2, (float)y2, r, g, b);
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
    
    // Левая панель
    drawPanel(0, 0, panelW, h, 0.2f, 0.2f, 0.2f);
    
    // Правая панель
    drawPanel(w - panelW, 0, w, h, 0.2f, 0.2f, 0.2f);
    
    // Верхняя панель
    drawPanel(panelW, 0, w - panelW, panelH, 0.3f, 0.3f, 0.3f);
    
    // Нижняя панель
    drawPanel(panelW, h - panelH, w - panelW, h, 0.3f, 0.3f, 0.3f);
    
    glEnd();
    
    restoreMatrices();
    restoreState(prog, vp, dt);
}