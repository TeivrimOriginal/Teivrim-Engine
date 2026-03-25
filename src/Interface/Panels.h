#ifndef PANELS_H
#define PANELS_H

#include "../Core/Render/Win32/RenderUI.h"

enum class PanelType {
    None,
    Left,
    Right,
    Top,
    Bottom
};

// Структура для хранения размеров панелей
struct PanelSizes {
    int leftWidth = 200;
    int rightWidth = 200;
    int topHeight = 50;
    int bottomHeight = 50;
    
    // Базовые минимальные размеры (будут обновляться из содержимого)
    int minLeftWidth = 50;
    int minRightWidth = 50;
    int minTopHeight = 30;
    int minBottomHeight = 30;
    
    int maxLeftWidth = 400;
    int maxRightWidth = 400;
    int maxTopHeight = 200;
    int maxBottomHeight = 200;
};

struct PanelDimensions {
    int leftPanelWidth;
    int rightPanelWidth;
    int topPanelHeight;
    int bottomPanelHeight;
    int centerX;
    int centerY;
    int centerWidth;
    int centerHeight;
};

class Panels {
private:
    PanelSizes sizes;
    
public:
    Panels();
    
    PanelDimensions getDimensions(int screenWidth, int screenHeight) const;
    void render(RenderUI& renderer, int screenWidth, int screenHeight) const;
    void getPanelBounds(PanelType panel, int screenWidth, int screenHeight, 
                        int& outX, int& outY, int& outW, int& outH) const;
    
    // Методы для изменения размеров с проверкой на минимальные значения
    void setLeftWidth(int width);
    void setRightWidth(int width);
    void setTopHeight(int height);
    void setBottomHeight(int height);
    
    // Обновление минимальных размеров на основе содержимого
    void updateMinSizes(int minLeft, int minRight, int minTop, int minBottom);
    
    // Геттеры
    int getLeftWidth() const { return sizes.leftWidth; }
    int getRightWidth() const { return sizes.rightWidth; }
    int getTopHeight() const { return sizes.topHeight; }
    int getBottomHeight() const { return sizes.bottomHeight; }
    int getMinLeftWidth() const { return sizes.minLeftWidth; }
    int getMinRightWidth() const { return sizes.minRightWidth; }
    int getMinTopHeight() const { return sizes.minTopHeight; }
    int getMinBottomHeight() const { return sizes.minBottomHeight; }
    
    // Проверка нахождения в зоне захвата
    bool isOnLeftEdge(int x, int y, int screenWidth, int screenHeight) const;
    bool isOnRightEdge(int x, int y, int screenWidth, int screenHeight) const;
    bool isOnTopEdge(int x, int y, int screenWidth, int screenHeight) const;
    bool isOnBottomEdge(int x, int y, int screenWidth, int screenHeight) const;
    
    PanelType getEdgeAt(int x, int y, int screenWidth, int screenHeight) const;
};

#endif