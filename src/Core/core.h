// core.h - полный
#ifndef CORE_H
#define CORE_H

#include <string>
#include <GL/glew.h>
#include <windows.h>
#include "Render/Win32/rendererw.h"
#include "Render/Parser/parser.h"
#include "Vulkan.h"

enum class RenderAPI { OPENGL, VULKAN };

class Core {
public:
    Core();
    bool isStart = false;
    bool modelLoaded = false;
    bool needsOptimize = false;
    std::string modelPath;
    GLuint shaderProgram = 0;
    
    void setRenderAPI(RenderAPI api);
    void initializeRender(InitialWin32* window);
    void renderModel(Camera& camera);
    void cleanupRender();
    void settingUpRender();
    void ParserToRender();
    void GameLoop();
    bool loadModelFromPath(const std::string& path);
    bool openFileDialogAndLoadModel(HWND hwnd);
    
    Vulkan* getVulkan() { return vulkan; }
    
    void SetViewportClip(int x, int y, int w, int h);
    void DisableViewportClip();
    bool IsViewportClippingEnabled() const { return viewportClipEnabled; }
    void GetViewportClip(int& x, int& y, int& w, int& h) const;
    
private:
    RendererW rendererw;
    RenderAPI currentAPI = RenderAPI::OPENGL;
    InitialWin32* currentWindow = nullptr;
    bool rendererInitialized = false;
    Vulkan* vulkan = nullptr;
    
    bool viewportClipEnabled = false;
    int clipX = 0, clipY = 0, clipW = 0, clipH = 0;
};

#endif