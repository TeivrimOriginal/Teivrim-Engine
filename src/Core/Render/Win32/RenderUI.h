#ifndef RENDERUI_H
#define RENDERUI_H

#include <GL/glew.h>
#include <windows.h>

class RenderUI {
public:
    RenderUI();
    ~RenderUI();
    
    // Основные методы рендеринга
    void drawUI(HWND hwnd);  // Рисует UI поверх всего
    
    // Методы для сохранения/восстановления состояния
    void saveState(GLint& prog, GLint vp[4], GLboolean& dt);
    void restoreState(GLint prog, GLint vp[4], GLboolean dt);
    
    // 2D настройки
    void setup2D(int width, int height);
    void restoreMatrices();
    
    // Рисование примитивов
    void drawQuad(float x1, float y1, float x2, float y2, float r, float g, float b);
    void drawQuad(int x1, int y1, int x2, int y2, float r, float g, float b);

private:
    void drawPanel(float x1, float y1, float x2, float y2, float r, float g, float b);
};

#endif