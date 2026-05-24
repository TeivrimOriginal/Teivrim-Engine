// SecondRender.cpp
#include "SecondRender.h"
#include "../Core/Vulkan.h"
#include "../Core/Render/Win32/RenderUI.h"
#include "../Interface/InterfaceManager.h"
#include "../Interface/Panels.h"
#include "../Core/SecondComplexity/Scene/SceneManager.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <float.h>

SecondRender::SecondRender() 
    : vulkan(nullptr)
    , renderUI(nullptr)
    , uiManager(nullptr)
    , screenW(1280)
    , screenH(720)
    , initialized(false)
    , backgroundEnabled(true)
    , overlayEnabled(true)
    , viewportX(0)
    , viewportY(0)
    , viewportW(0)
    , viewportH(0)
    , cameraMatrixValid(false)
{
    gridConfig.cellSize = 50;
    gridConfig.gridSize = 20;
    gridConfig.lineColor[0] = 0.4f;
    gridConfig.lineColor[1] = 0.4f;
    gridConfig.lineColor[2] = 0.45f;
    gridConfig.centerLineColor[0] = 0.8f;
    gridConfig.centerLineColor[1] = 0.8f;
    gridConfig.centerLineColor[2] = 1.0f;
    gridConfig.axisXColor[0] = 1.0f;
    gridConfig.axisXColor[1] = 0.0f;
    gridConfig.axisXColor[2] = 0.0f;
    gridConfig.axisZColor[0] = 0.0f;
    gridConfig.axisZColor[1] = 1.0f;
    gridConfig.axisZColor[2] = 0.0f;
    gridConfig.enabled = true;
    gridConfig.infiniteGrid = true;
    gridConfig.showAxes = true;
    gridConfig.gridSpacing = 20.0f;
    gridConfig.fadeDistance = 500.0f;
    gridConfig.yOffset = 0.0f;
    gridConfig.lineThickness = 1.0f;
    
    std::cout << "[SecondRender] Constructor called" << std::endl;
}

SecondRender::~SecondRender() {
    ClearBackground();
    ClearOverlay();
}

void SecondRender::Initialize(Vulkan* vk, RenderUI* ui, InterfaceManager* manager, int screenWidth, int screenHeight) {
    vulkan = vk;
    renderUI = ui;
    uiManager = manager;
    screenW = screenWidth;
    screenH = screenHeight;
    initialized = true;
    
    std::cout << "[SecondRender] Initialize called, vulkan=" << (vulkan ? "OK" : "NULL") << std::endl;
    
    UpdateViewportRect();
}

void SecondRender::UpdateViewportRect() {
    if (!uiManager) {
        viewportX = 0;
        viewportY = 0;
        viewportW = screenW;
        viewportH = screenH;
        return;
    }
    
    Panel* view3D = uiManager->getPanelManager()->get3D();
    if (view3D && view3D->visible && !view3D->collapsed) {
        viewportX = view3D->getX();
        viewportY = view3D->getY();
        viewportW = view3D->getW();
        viewportH = view3D->getH();
        
        if (viewportX < 0) viewportX = 0;
        if (viewportY < 0) viewportY = 0;
        if (viewportX + viewportW > screenW) viewportW = screenW - viewportX;
        if (viewportY + viewportH > screenH) viewportH = screenH - viewportY;
    } else {
        viewportX = 0;
        viewportY = 0;
        viewportW = screenW;
        viewportH = screenH;
    }
    
    testQuadsDirty = true;
}

void SecondRender::DrawBackgroundQuad(float x1, float y1, float x2, float y2, float r, float g, float b) {
    if (!backgroundEnabled) return;
    
    Quad2D quad;
    quad.x1 = x1 + viewportX;
    quad.y1 = y1 + viewportY;
    quad.x2 = x2 + viewportX;
    quad.y2 = y2 + viewportY;
    quad.r = r; quad.g = g; quad.b = b;
    quad.useTexture = false;
    quad.textureId = nullptr;
    quad.layer = 0;
    quad.u1 = quad.v1 = 0.0f;
    quad.u2 = quad.v2 = 1.0f;
    
    backgroundQuads.push_back(quad);
}

void SecondRender::DrawOverlayQuad(float x1, float y1, float x2, float y2, float r, float g, float b) {
    if (!overlayEnabled) return;
    
    Quad2D quad;
    quad.x1 = x1 + viewportX;
    quad.y1 = y1 + viewportY;
    quad.x2 = x2 + viewportX;
    quad.y2 = y2 + viewportY;
    quad.r = r; quad.g = g; quad.b = b;
    quad.useTexture = false;
    quad.textureId = nullptr;
    quad.layer = 1;
    quad.u1 = quad.v1 = 0.0f;
    quad.u2 = quad.v2 = 1.0f;
    
    overlayQuads.push_back(quad);
}

void SecondRender::DrawBackgroundImage(float x1, float y1, float x2, float y2, void* texture) {
    if (!backgroundEnabled || !texture) return;
    
    Quad2D quad;
    quad.x1 = x1 + viewportX;
    quad.y1 = y1 + viewportY;
    quad.x2 = x2 + viewportX;
    quad.y2 = y2 + viewportY;
    quad.r = quad.g = quad.b = 1.0f;
    quad.useTexture = true;
    quad.textureId = texture;
    quad.layer = 0;
    quad.u1 = 0.0f; quad.v1 = 0.0f;
    quad.u2 = 1.0f; quad.v2 = 1.0f;
    
    backgroundQuads.push_back(quad);
}

void SecondRender::DrawOverlayImage(float x1, float y1, float x2, float y2, void* texture) {
    if (!overlayEnabled || !texture) return;
    
    Quad2D quad;
    quad.x1 = x1 + viewportX;
    quad.y1 = y1 + viewportY;
    quad.x2 = x2 + viewportX;
    quad.y2 = y2 + viewportY;
    quad.r = quad.g = quad.b = 1.0f;
    quad.useTexture = true;
    quad.textureId = texture;
    quad.layer = 1;
    quad.u1 = 0.0f; quad.v1 = 0.0f;
    quad.u2 = 1.0f; quad.v2 = 1.0f;
    
    overlayQuads.push_back(quad);
}

void SecondRender::DrawGrid(int cellSize, int gridSize) {
    if (!backgroundEnabled || !gridConfig.enabled) return;
    
    gridConfig.cellSize = cellSize;
    gridConfig.gridSize = gridSize;
}

void SecondRender::SetGridConfig(const GridConfig& config) {
    gridConfig = config;
}

void SecondRender::SetCamera(const glm::mat4& viewMatrix, const glm::mat4& projMatrix, const glm::vec3& cameraPos) {
    currentViewMat = viewMatrix;
    currentProjMat = projMatrix;
    currentCameraPos = cameraPos;
    cameraMatrixValid = true;
}

std::vector<std::pair<glm::vec3, glm::vec3>> SecondRender::CalculateGridLines() {
    std::vector<std::pair<glm::vec3, glm::vec3>> lines;
    
    if (!gridConfig.infiniteGrid) return lines;
    
    float spacing = GetDynamicSpacing();
    float yOffset = gridConfig.yOffset;
    
    float distance = glm::length(currentCameraPos);
    float worldSize = std::max(200.0f, distance * 1.5f);
    float worldMin = -worldSize;
    float worldMax = worldSize;
    
    float minX = std::max(worldMin, -500.0f);
    float maxX = std::min(worldMax, 500.0f);
    float minZ = std::max(worldMin, -500.0f);
    float maxZ = std::min(worldMax, 500.0f);
    
    minX = floor(minX / spacing) * spacing;
    maxX = ceil(maxX / spacing) * spacing;
    minZ = floor(minZ / spacing) * spacing;
    maxZ = ceil(maxZ / spacing) * spacing;
    
    for (float x = minX; x <= maxX + 0.1f; x += spacing) {
        lines.push_back({glm::vec3(x, yOffset, minZ), glm::vec3(x, yOffset, maxZ)});
    }
    
    for (float z = minZ; z <= maxZ + 0.1f; z += spacing) {
        lines.push_back({glm::vec3(minX, yOffset, z), glm::vec3(maxX, yOffset, z)});
    }
    
    return lines;
}

std::vector<std::pair<glm::vec3, glm::vec3>> SecondRender::CalculateAxesLines() {
    std::vector<std::pair<glm::vec3, glm::vec3>> lines;
    
    float axisLength = gridConfig.fadeDistance;
    float yOffset = gridConfig.yOffset;
    
    lines.push_back({glm::vec3(-axisLength, yOffset, 0.0f), glm::vec3(axisLength, yOffset, 0.0f)});
    lines.push_back({glm::vec3(0.0f, yOffset, -axisLength), glm::vec3(0.0f, yOffset, axisLength)});
    
    return lines;
}

void SecondRender::RenderInfiniteGrid() {
    if (!gridConfig.enabled || !gridConfig.infiniteGrid) return;
    if (!cameraMatrixValid) return;
}

float SecondRender::GetDynamicSpacing() {
    float baseSpacing = 5.0f;
    float distance = glm::length(currentCameraPos);
    float zoomFactor = std::max(0.5f, std::min(10.0f, distance / 20.0f));
    
    if (zoomFactor < 1.0f) return baseSpacing * 0.5f;
    if (zoomFactor < 2.0f) return baseSpacing;
    if (zoomFactor < 4.0f) return baseSpacing * 2.0f;
    if (zoomFactor < 8.0f) return baseSpacing * 4.0f;
    return baseSpacing * 8.0f;
}

void SecondRender::SetZoomLevel(float zoom) {
    currentZoom = zoom;
}

void SecondRender::RenderContour(ObjectID objectId, float thickness, float r, float g, float b) {
    // Устаревший метод - контуры теперь через PostRender
    // Оставлен для обратной совместимости, но ничего не делает
}

void SecondRender::RenderBackground() {
    if (!backgroundEnabled || !vulkan) return;
    
    UpdateViewportRect();
    RebuildTestQuadsIfNeeded();
    
    std::vector<Quad2D> quadsToRender;
    quadsToRender.insert(quadsToRender.end(), testBackgroundQuads.begin(), testBackgroundQuads.end());
    quadsToRender.insert(quadsToRender.end(), backgroundQuads.begin(), backgroundQuads.end());
    
    for (const auto& quad : quadsToRender) {
        if (quad.useTexture && quad.textureId) {
            vulkan->drawImageUV(quad.x1, quad.y1, quad.x2, quad.y2, 
                                (VulkanTexture*)quad.textureId,
                                quad.u1, quad.v1, quad.u2, quad.v2);
        } else {
            vulkan->drawBackground(quad.x1, quad.y1, quad.x2, quad.y2, quad.r, quad.g, quad.b);
        }
    }
    
    backgroundQuads.clear();
}

void SecondRender::RenderOverlay() {
    if (!overlayEnabled || !vulkan) return;
    
    UpdateViewportRect();
    RebuildTestQuadsIfNeeded();
    
    std::vector<Quad2D> quadsToRender;
    quadsToRender.insert(quadsToRender.end(), testOverlayQuads.begin(), testOverlayQuads.end());
    quadsToRender.insert(quadsToRender.end(), overlayQuads.begin(), overlayQuads.end());
    
    for (const auto& quad : quadsToRender) {
        if (quad.useTexture && quad.textureId) {
            vulkan->drawImageUV(quad.x1, quad.y1, quad.x2, quad.y2, 
                                (VulkanTexture*)quad.textureId,
                                quad.u1, quad.v1, quad.u2, quad.v2);
        } else {
            vulkan->drawQuad(quad.x1, quad.y1, quad.x2, quad.y2, quad.r, quad.g, quad.b);
        }
    }
    
    overlayQuads.clear();
}

void SecondRender::ClearBackground() {
    backgroundQuads.clear();
}

void SecondRender::ClearOverlay() {
    overlayQuads.clear();
}

void SecondRender::UpdateScreenSize(int width, int height) {
    screenW = width;
    screenH = height;
    UpdateViewportRect();
    testQuadsDirty = true;
}

void SecondRender::RebuildTestQuadsIfNeeded() {
    if (!testQuadsDirty) return;
    
    testBackgroundQuads.clear();
    testOverlayQuads.clear();
    
    Quad2D quad;
    
    quad.useTexture = false;
    quad.textureId = nullptr;
    quad.layer = 0;
    quad.x1 = 200.0f + viewportX;
    quad.y1 = 200.0f + viewportY;
    quad.x2 = 300.0f + viewportX;
    quad.y2 = 300.0f + viewportY;
    quad.r = 0.3f; quad.g = 0.3f; quad.b = 0.4f;
    testBackgroundQuads.push_back(quad);
    
    quad.layer = 1;
    quad.x1 = 50.0f + viewportX;
    quad.y1 = 50.0f + viewportY;
    quad.x2 = 150.0f + viewportX;
    quad.y2 = 150.0f + viewportY;
    quad.r = 1.0f; quad.g = 1.0f; quad.b = 1.0f;
    testOverlayQuads.push_back(quad);
    
    quad.x1 = (viewportW - 150.0f) + viewportX;
    quad.y1 = 50.0f + viewportY;
    quad.x2 = (viewportW - 50.0f) + viewportX;
    quad.y2 = 150.0f + viewportY;
    testOverlayQuads.push_back(quad);
    
    testQuadsDirty = false;
}

void SecondRender::SetContourEnabled(bool enabled) {
    // Устаревший метод - контуры теперь через PostRender
    // Оставлен для обратной совместимости
    std::cout << "[SecondRender] SetContourEnabled(" << enabled << ") - Deprecated, use PostRender instead" << std::endl;
}