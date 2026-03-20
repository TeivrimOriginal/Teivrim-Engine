#include "Panels.h"

Panels::Panels() {}

PanelDimensions Panels::getDimensions(int sw, int sh) const {
    PanelDimensions d;
    d.leftPanelWidth = (int)(sw * 0.156f);
    d.rightPanelWidth = (int)(sw * 0.156f);
    d.topPanelHeight = (int)(sh * 0.046f);
    d.bottomPanelHeight = (int)(sh * 0.046f);
    
    d.leftPanelX1 = 0; 
    d.leftPanelY1 = 0; 
    d.leftPanelX2 = d.leftPanelWidth; 
    d.leftPanelY2 = sh;
    
    d.rightPanelX1 = sw - d.rightPanelWidth; 
    d.rightPanelY1 = 0; 
    d.rightPanelX2 = sw; 
    d.rightPanelY2 = sh;
    
    d.topPanelX1 = d.leftPanelWidth; 
    d.topPanelY1 = 0; 
    d.topPanelX2 = sw - d.rightPanelWidth; 
    d.topPanelY2 = d.topPanelHeight;
    
    d.bottomPanelX1 = d.leftPanelWidth; 
    d.bottomPanelY1 = sh - d.bottomPanelHeight; 
    d.bottomPanelX2 = sw - d.rightPanelWidth; 
    d.bottomPanelY2 = sh;
    
    d.toolPanelX1 = (sw - 100) / 2; 
    d.toolPanelY1 = (d.topPanelHeight - 30) / 2;
    d.toolPanelX2 = d.toolPanelX1 + 100; 
    d.toolPanelY2 = d.toolPanelY1 + 30;
    
    return d;
}

void Panels::render(RenderUI& r, int sw, int sh) const {
    PanelDimensions d = getDimensions(sw, sh);
    
    glBegin(GL_QUADS);
    
    // Левая панель (темно-серая)
    r.drawQuad(d.leftPanelX1, d.leftPanelY1, d.leftPanelX2, d.leftPanelY2, 0.2f, 0.2f, 0.2f);
    
    // Правая панель (темно-серая)
    r.drawQuad(d.rightPanelX1, d.rightPanelY1, d.rightPanelX2, d.rightPanelY2, 0.2f, 0.2f, 0.2f);
    
    // Верхняя панель (светло-серая)
    r.drawQuad(d.topPanelX1, d.topPanelY1, d.topPanelX2, d.topPanelY2, 0.3f, 0.3f, 0.3f);
    
    // Нижняя панель (светло-серая)
    r.drawQuad(d.bottomPanelX1, d.bottomPanelY1, d.bottomPanelX2, d.bottomPanelY2, 0.3f, 0.3f, 0.3f);
    
    glEnd();
}

PanelType Panels::getPanelAtPosition(int x, int y, int sw, int sh) const {
    PanelDimensions d = getDimensions(sw, sh);
    
    if (x >= d.leftPanelX1 && x <= d.leftPanelX2) 
        return PanelType::Left;
    if (x >= d.rightPanelX1 && x <= d.rightPanelX2) 
        return PanelType::Right;
    if (y >= d.topPanelY1 && y <= d.topPanelY2) 
        return PanelType::Top;
    if (y >= d.bottomPanelY1 && y <= d.bottomPanelY2) 
        return PanelType::Bottom;
    
    return PanelType::None;
}

void Panels::getPanelBounds(PanelType p, int sw, int sh, int& x1, int& y1, int& x2, int& y2) const {
    PanelDimensions d = getDimensions(sw, sh);
    
    switch(p) {
        case PanelType::Left:
            x1 = d.leftPanelX1; y1 = d.leftPanelY1; 
            x2 = d.leftPanelX2; y2 = d.leftPanelY2; 
            break;
        case PanelType::Right:
            x1 = d.rightPanelX1; y1 = d.rightPanelY1; 
            x2 = d.rightPanelX2; y2 = d.rightPanelY2; 
            break;
        case PanelType::Top:
            x1 = d.topPanelX1; y1 = d.topPanelY1; 
            x2 = d.topPanelX2; y2 = d.topPanelY2; 
            break;
        case PanelType::Bottom:
            x1 = d.bottomPanelX1; y1 = d.bottomPanelY1; 
            x2 = d.bottomPanelX2; y2 = d.bottomPanelY2; 
            break;
        default:
            x1 = y1 = x2 = y2 = 0;
    }
}