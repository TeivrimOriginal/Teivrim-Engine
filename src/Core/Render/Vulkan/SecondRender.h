// SecondRender.h
#ifndef SECOND_RENDER_H
#define SECOND_RENDER_H

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "Vulkan.h"
#include "../RenderUI.h"

class Vulkan;
class RenderUI;
class InterfaceManager;
struct VulkanTexture;

typedef unsigned int ObjectID;

struct GridConfig {
    int cellSize;
    int gridSize;
    float lineColor[3];
    float centerLineColor[3];
    float axisXColor[3];
    float axisZColor[3];
    bool enabled;
    bool infiniteGrid;
    bool showAxes;
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
    
    void SetZoomLevel(float zoom);
    float GetZoomLevel() const { return currentZoom; }
    
    // Устаревшие методы - контуры теперь через PostRender
    void RenderContour(ObjectID objectId, float thickness = 3.0f, 
                       float r = 1.0f, float g = 0.5f, float b = 0.0f);
    void SetContourEnabled(bool enabled);
    
    void DrawGrid(int cellSize = 50, int gridSize = 20);
    
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
    
    float currentZoom = 1.0f;
    bool contourEnabled = true;
    float contourThickness = 3.0f;
    float contourColor[3] = {1.0f, 0.5f, 0.0f};
    
    void RebuildTestQuadsIfNeeded();
    
    std::vector<std::pair<glm::vec3, glm::vec3>> CalculateGridLines();
    std::vector<std::pair<glm::vec3, glm::vec3>> CalculateAxesLines();
    float GetDynamicSpacing();
};

#endif