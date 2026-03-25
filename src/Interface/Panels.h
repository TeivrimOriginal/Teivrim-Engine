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
    int leftWidth = 200;      // Ширина левой панели
    int rightWidth = 200;     // Ширина правой панели
    int topHeight = 50;       // Высота верхней панели
    int bottomHeight = 50;    // Высота нижней панели
    
    int minLeftWidth = 50;    // Минимальная ширина левой
    int minRightWidth = 50;   // Минимальная ширина правой
    int minTopHeight = 30;    // Минимальная высота верхней
    int minBottomHeight = 30; // Минимальная высота нижней
    
    int maxLeftWidth = 400;   // Максимальная ширина левой
    int maxRightWidth = 400;  // Максимальная ширина правой
    int maxTopHeight = 200;   // Максимальная высота верхней
    int maxBottomHeight = 200; // Максимальная высота нижней
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
    
    // Получить размеры с учетом текущих настроек
    PanelDimensions getDimensions(int screenWidth, int screenHeight) const;
    
    // Отрисовать панели
    void render(RenderUI& renderer, int screenWidth, int screenHeight) const;
    
    // Получить границы панели
    void getPanelBounds(PanelType panel, int screenWidth, int screenHeight, 
                        int& outX, int& outY, int& outW, int& outH) const;
    
    // Методы для изменения размеров
    void setLeftWidth(int width);
    void setRightWidth(int width);
    void setTopHeight(int height);
    void setBottomHeight(int height);
    
    // Геттеры для размеров
    int getLeftWidth() const { return sizes.leftWidth; }
    int getRightWidth() const { return sizes.rightWidth; }
    int getTopHeight() const { return sizes.topHeight; }
    int getBottomHeight() const { return sizes.bottomHeight; }
    
    // Проверка нахождения в зоне захвата грани
    bool isOnLeftEdge(int x, int y, int screenWidth, int screenHeight) const;
    bool isOnRightEdge(int x, int y, int screenWidth, int screenHeight) const;
    bool isOnTopEdge(int x, int y, int screenWidth, int screenHeight) const;
    bool isOnBottomEdge(int x, int y, int screenWidth, int screenHeight) const;
    
    // Получить тип грани по координатам
    PanelType getEdgeAt(int x, int y, int screenWidth, int screenHeight) const;
};

#endif