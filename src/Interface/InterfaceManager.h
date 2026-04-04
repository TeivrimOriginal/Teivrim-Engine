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

// Флаг для выбора API рендеринга
enum class RenderAPI {
    OPENGL,
    VULKAN
};

class InterfaceManager {
private:
    InitialWin32* window;
    RenderUI renderer;
    ObjectUI objectUI;
    PanelManager* panels;
    Core* core;
    RenderAPI currentAPI;
    
public:
    InterfaceManager(Core* corePtr, RenderAPI api = RenderAPI::OPENGL);
    ~InterfaceManager();
    
    void setWindow(InitialWin32* w) { window = w; }
    Dimensions getDimensions();
    void clearScreen(int width, int height);
    void setup3DViewport(const Dimensions& dims);
    void renderStatic();
    void renderDynamic();
    void handleClick(int x, int y);
    void handleMouseMove(int x, int y);
    void handleMouseDown(int x, int y); 
    void handleMouseUp(int x, int y);
    void SwapFlag(Core &A);
    HWND getHWND() const;
    
    bool initializeRender(HWND hwnd, int width, int height);
    void beginFrame();
    void endFrame();
    void present();
    
    bool isClick = true;
    void swapclick() { isClick = !isClick; }
    void BlockMoveToMainWindow(int x, int y);
    bool CheckerClickToPanel(int x, int y);
    
    bool isBlockingRender() { return panels ? panels->isBlockingInput() : false; }
    PanelManager* getPanelManager() { return panels; }
    
    RenderAPI getCurrentAPI() const { return currentAPI; }
    void setRenderAPI(RenderAPI api);
};

#endif