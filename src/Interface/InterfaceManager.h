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

public:
    InterfaceManager(Core* corePtr);
    void setWindow(InitialWin32* w) { window = w; }
    
    Dimensions getDimensions();
    void clearScreen(int width, int height);
    void setup3DViewport(const Dimensions& dims);
    void renderStatic();
    void renderDynamic();
    void handleClick(int x, int y);
    void SwapFlag(Core &A);
};

#endif