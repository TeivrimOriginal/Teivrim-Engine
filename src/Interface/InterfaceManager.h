#ifndef INTERFACE_MANAGER_H
#define INTERFACE_MANAGER_H

#include <windows.h>
#include "../Core/Render/Win32/RenderUI.h"
#include "Panels.h"
#include "ObjectUI.h"
#include "../Core/core.h"

class InterfaceManager {
public:
    InterfaceManager(Core* corePtr, RenderAPI api);
    ~InterfaceManager();

    void initializeRender(HWND hwnd, int width, int height);
    void renderStatic();
    void renderDynamic();
    void handleClick(int x, int y);
    void handleMouseMove(int x, int y);
    void handleMouseDown(int x, int y);
    void handleMouseUp(int x, int y);
    void updateWindowSize(int width, int height);
    bool isBlockingRender();
    void setWindow(InitialWin32* window);
    PanelManager* getPanelManager() { return panels; }
    void SwapFlag(Core &A);
    HWND getHWND() const;
    void BlockMoveToMainWindow(int x, int y);
    bool CheckerClickToPanel(int x, int y);
    bool isClick = false;  // Добавляем isClick
    
    struct Dimensions {
        int width, height;
    };
    Dimensions getDimensions();
    void clearScreen(int width, int height);
    void setup3DViewport(const Dimensions& dims);
    void setCore(Core* corePtr) { core = corePtr; }

private:
    InitialWin32* window;
    Core* core;
    PanelManager* panels;
    ObjectUI objectUI;
    RenderUI renderer;
    RenderAPI currentAPI;
    bool startupActive;
};

#endif