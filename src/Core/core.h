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
    ModelParser modelParser;
    
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
    
private:
    RendererW rendererw;
    RenderAPI currentAPI = RenderAPI::OPENGL;
    InitialWin32* currentWindow = nullptr;
    bool rendererInitialized = false;
    Vulkan* vulkan = nullptr;
};

#endif