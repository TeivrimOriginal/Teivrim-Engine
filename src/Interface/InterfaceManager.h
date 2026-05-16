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
    ObjectUI& getObjectUI() { return objectUI; }
    Core* getCore() { return core; }
    void SwapFlag(Core &A);
    HWND getHWND() const;
    void BlockMoveToMainWindow(int x, int y);
    bool CheckerClickToPanel(int x, int y);
    bool isClick = false;
    
    void printIcon(int x, int y, int w, int h, const std::string& iconType, int size = 64);
    RenderAPI getCurrentAPI() const { return currentAPI; }
    void setTestTexture(VulkanTexture* tex) { testTexture = tex; }
    void setCore(Core* corePtr) { core = corePtr; }
    
    // Метод для отладки - выводит иерархию в консоль
    void PrintHierarchy() const;
    
    struct Dimensions {
        int width, height;
    };
    Dimensions getDimensions() const;
    void clearScreen(int width, int height);
    void setup3DViewport(const Dimensions& dims);
    
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