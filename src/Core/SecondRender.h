#ifndef SECOND_RENDER_H
#define SECOND_RENDER_H

#include <windows.h>
#include <vector>
#include <string>
#include <iostream>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

class Vulkan;
class RenderUI;
class InterfaceManager;
struct VulkanTexture;

struct GridConfig {
    int cellSize;
    int gridSize;
    float lineColor[3];
    float centerLineColor[3];
    bool enabled;
    bool infiniteGrid;
    float gridSpacing;
    float fadeDistance;
    float yOffset;
    float lineThickness;
};

struct Quad2D {
    float x1, y1, x2, y2;
    float r, g, b;
    float u1, v1, u2, v2;
    bool useTexture;
    void* textureId;
    int layer;
};

struct LineVertex {
    glm::vec2 pos;
    glm::vec3 color;
};

class SecondRender {
public:
    static SecondRender& Instance() {
        static SecondRender instance;
        return instance;
    }
    
    void Initialize(Vulkan* vk, RenderUI* ui, InterfaceManager* uiManager, int screenWidth, int screenHeight);
    
    void SetBackgroundEnabled(bool enabled) { backgroundEnabled = enabled; }
    void SetOverlayEnabled(bool enabled) { overlayEnabled = enabled; }
    
    void DrawBackgroundQuad(float x1, float y1, float x2, float y2, float r, float g, float b);
    void DrawOverlayQuad(float x1, float y1, float x2, float y2, float r, float g, float b);
    
    void SetGridConfig(const GridConfig& config);
    void SetCamera(const glm::mat4& viewMatrix, const glm::mat4& projMatrix, const glm::vec3& cameraPos);
    
    void MarkTestQuadsDirty() { testQuadsDirty = true; }
    
    void RenderBackground();
    void RenderOverlay();
    void RenderInfiniteGrid();
    
    void ClearBackground();
    void ClearOverlay();
    
    void UpdateScreenSize(int width, int height);
    void UpdateViewportRect();
    
    bool IsInitialized() const { return initialized; }
    
private:
    SecondRender();
    ~SecondRender();
    
    void DestroyLineResources();
    bool CreateLinePipeline();
    void UpdateGridBuffer();
    
    Vulkan* vulkan;
    RenderUI* renderUI;
    InterfaceManager* uiManager;
    int screenW, screenH;
    bool initialized;
    bool backgroundEnabled;
    bool overlayEnabled;
    
    int viewportX, viewportY, viewportW, viewportH;
    
    std::vector<Quad2D> backgroundQuads;
    std::vector<Quad2D> overlayQuads;
    GridConfig gridConfig;
    
    bool testQuadsDirty = true;
    std::vector<Quad2D> testBackgroundQuads;
    std::vector<Quad2D> testOverlayQuads;
    
    glm::mat4 currentViewMat;
    glm::mat4 currentProjMat;
    glm::vec3 currentCameraPos;
    bool cameraMatrixValid;
    bool needBufferUpdate;
    float lastCamX, lastCamZ;
    
    VkPipeline linePipeline;
    VkPipelineLayout linePipelineLayout;
    VkBuffer lineVertexBuffer;
    VkDeviceMemory lineVertexBufferMemory;
    uint32_t lineVertexCount;
    
    void RebuildTestQuadsIfNeeded();
    std::vector<std::pair<glm::vec3, glm::vec3>> CalculateGridLines();
};

#endif