// InterfaceManager.h - FULL
#ifndef INTERFACE_MANAGER_H
#define INTERFACE_MANAGER_H

#include <windows.h>
#include "../Core/Render/Win32/RenderUI.h"
#include "Panels.h"
#include "ObjectUI.h"

class Core;
class Vulkan;
struct VulkanTexture;
enum class RenderAPI;

class InterfaceManager {
public:
    Vulkan* getVulkan() { return renderer.getVulkan(); }
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
    
    void printIcon(int x, int y, int w, int h, const std::string& iconType, int size = 64);
    RenderAPI getCurrentAPI() const { return currentAPI; }
    void setTestTexture(VulkanTexture* tex) { testTexture = tex; }
    Core* getCore() { return core; }
    void setCore(Core* corePtr) { core = corePtr; }
    
    struct Dimensions {
        int width, height;
    };
    Dimensions getDimensions() const;
    void clearScreen(int width, int height);
    void setup3DViewport(const Dimensions& dims);
    
    // Получить текущую область 3D Viewport
    void Get3DViewportRect(int& x, int& y, int& w, int& h) const;

private:
    class InitialWin32* window;
    Core* core;
    PanelManager* panels;
    ObjectUI objectUI;
    RenderUI renderer;
    RenderAPI currentAPI;
    bool startupActive;
    VulkanTexture* testTexture = nullptr;
};

#endif