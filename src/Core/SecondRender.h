// SecondRender.h - оставляем только это определение GridConfig
#ifndef SECOND_RENDER_H
#define SECOND_RENDER_H

#include <windows.h>
#include <vector>
#include <string>
#include <functional>
#include <iostream>
#include <glm/glm.hpp>

class Vulkan;
class RenderUI;
class InterfaceManager;
class Panel;
struct VulkanTexture;
class Camera;

// ЕДИНСТВЕННОЕ ОПРЕДЕЛЕНИЕ GridConfig
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
};

struct Quad2D {
    float x1, y1, x2, y2;
    float r, g, b;
    float u1, v1, u2, v2;
    bool useTexture;
    void* textureId;
    int layer;
};

struct GridLine {
    glm::vec3 start;
    glm::vec3 end;
    float alpha;
    bool isCenter;
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
    void DrawBackgroundImage(float x1, float y1, float x2, float y2, void* texture);
    void DrawOverlayImage(float x1, float y1, float x2, float y2, void* texture);
    
    void DrawGrid(int cellSize = 50, int gridSize = 20);
    void SetGridConfig(const GridConfig& config);
    void SetCamera(const glm::mat4& viewMatrix, const glm::mat4& projMatrix, const glm::vec3& cameraPos);
    
    void DrawTestQuads();
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
    
    void DrawGridInternal();
    void RebuildTestQuadsIfNeeded();
    std::vector<GridLine> CalculateGridLines();
    void DrawInfiniteGridLines(const std::vector<GridLine>& lines);
};

#endif