#ifndef PANELS_H
#define PANELS_H

#include "../Core/Render/Win32/RenderUI.h"

enum class PanelType { None, Left, Right, Top, Bottom, Tool };

struct PanelDimensions {
    int leftPanelWidth, rightPanelWidth, topPanelHeight, bottomPanelHeight;
    int leftPanelX1, leftPanelY1, leftPanelX2, leftPanelY2;
    int rightPanelX1, rightPanelY1, rightPanelX2, rightPanelY2;
    int topPanelX1, topPanelY1, topPanelX2, topPanelY2;
    int bottomPanelX1, bottomPanelY1, bottomPanelX2, bottomPanelY2;
    int toolPanelX1, toolPanelY1, toolPanelX2, toolPanelY2;
};

class Panels {
public:
    Panels();
    
    PanelDimensions getDimensions(int screenWidth, int screenHeight) const;
    void render(RenderUI& renderer, int screenWidth, int screenHeight) const;
    PanelType getPanelAtPosition(int x, int y, int screenWidth, int screenHeight) const;
    void getPanelBounds(PanelType panel, int screenWidth, int screenHeight, 
                       int& x1, int& y1, int& x2, int& y2) const;
};

#endif