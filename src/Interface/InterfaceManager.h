#ifndef INTERFACEMANAGER_H
#define INTERFACEMANAGER_H

#include "../core/Render/Win32/RenderUI.h"
#include "ObjectUI.h"
#include "Panels.h"
#include "../Application/WindowAPIsupport/Win32/InitialWin32.h"
#include "../Core/core.h"

struct Dimensions {
    int width;
    int height;
};

class InterfaceManager {
private:
    InitialWin32* window;
    RenderUI renderer;
    ObjectUI objectUI;
    Panels panels;
    Core* core;
    
    // Состояние для перетаскивания граней
    bool isDragging = false;
    PanelType draggingEdge = PanelType::None;
    int dragStartX = 0;
    int dragStartY = 0;
    int dragStartValue = 0;  // Начальный размер панели
    
public:
    InterfaceManager(Core* corePtr);
    void setWindow(InitialWin32* w) { window = w; }
    
    Dimensions getDimensions();
    void clearScreen(int width, int height);
    void setup3DViewport(const Dimensions& dims);
    void renderStatic();
    void renderDynamic();
    void handleClick(int x, int y);
    void handleMouseMove(int x, int y);  // Новый метод
    void handleMouseDown(int x, int y);   // Новый метод
    void handleMouseUp(int x, int y);     // Новый метод
    void SwapFlag(Core &A);
    HWND getHWND() const;
    
    // Получить текущий курсор для грани
    HCURSOR getCursorForEdge(PanelType edge) const;
};

#endif