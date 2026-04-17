#ifndef INTERFACE_MANAGER_H
#define INTERFACE_MANAGER_H

#include <windows.h>
#include "../Core/Render/Win32/RenderUI.h"
#include "Panels.h"
#include "ObjectUI.h"

class Core;
class Vulkan;
enum class RenderAPI;

class InterfaceManager {
public:
    InterfaceManager(Core* corePtr, RenderAPI api);
    ~InterfaceManager();

    void initializeRender(HWND hwnd, int width, int height);
    void renderStatic();
    void renderDynamic();
    void handleClick(int x, int y);
    void handleRightClick(int x, int y);
    void handleMouseMove(int x, int y);
    void handleMouseDown(int x, int y);
    void handleMouseUp(int x, int y);
    void updateWindowSize(int width, int height);
    bool isBlockingRender();
    void setWindow(class InitialWin32* window);
    void setVulkan(class Vulkan* vk);
    PanelManager* getPanelManager() { return panels; }
    RenderUI& getRenderer() { return renderer; }
    void SwapFlag(Core &A);
    HWND getHWND() const;
    void BlockMoveToMainWindow(int x, int y);
    bool CheckerClickToPanel(int x, int y);
    bool isClick = false;
    
    struct Dimensions {
        int width, height;
    };
    Dimensions getDimensions();
    void clearScreen(int width, int height);
    void setup3DViewport(const Dimensions& dims);
    void setCore(Core* corePtr) { core = corePtr; }

private:
    class InitialWin32* window;
    Core* core;
    PanelManager* panels;
    ObjectUI objectUI;
    RenderUI renderer;
    RenderAPI currentAPI;
    bool startupActive;
};

#endif