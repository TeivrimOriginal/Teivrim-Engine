// SecondRender.cpp
#include "SecondRender.h"
#include "Vulkan.h"
#include "Render/Win32/RenderUI.h"
#include "../Interface/InterfaceManager.h"

SecondRender::SecondRender() 
    : vulkan(nullptr)
    , renderUI(nullptr)
    , screenW(1280)
    , screenH(720)
    , initialized(false)
    , backgroundEnabled(true)
    , overlayEnabled(true)
{
    gridConfig.cellSize = 50;
    gridConfig.gridSize = 20;
    gridConfig.lineColor[0] = 0.3f;
    gridConfig.lineColor[1] = 0.3f;
    gridConfig.lineColor[2] = 0.35f;
    gridConfig.centerLineColor[0] = 0.6f;
    gridConfig.centerLineColor[1] = 0.6f;
    gridConfig.centerLineColor[2] = 0.7f;
    gridConfig.enabled = true;
}

SecondRender::~SecondRender() {
    ClearBackground();
    ClearOverlay();
}

void SecondRender::Initialize(Vulkan* vk, RenderUI* ui, int screenWidth, int screenHeight) {
    vulkan = vk;
    renderUI = ui;
    screenW = screenWidth;
    screenH = screenHeight;
    initialized = (vulkan != nullptr);
    
    if (initialized) {
        DrawTestQuads();
    }
}

void SecondRender::DrawBackgroundQuad(float x1, float y1, float x2, float y2, float r, float g, float b) {
    if (!backgroundEnabled) return;
    
    Quad2D quad;
    quad.x1 = x1; quad.y1 = y1;
    quad.x2 = x2; quad.y2 = y2;
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
    quad.x1 = x1; quad.y1 = y1;
    quad.x2 = x2; quad.y2 = y2;
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
    quad.x1 = x1; quad.y1 = y1;
    quad.x2 = x2; quad.y2 = y2;
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
    quad.x1 = x1; quad.y1 = y1;
    quad.x2 = x2; quad.y2 = y2;
    quad.r = quad.g = quad.b = 1.0f;
    quad.useTexture = true;
    quad.textureId = texture;
    quad.layer = 1;
    quad.u1 = 0.0f; quad.v1 = 0.0f;
    quad.u2 = 1.0f; quad.v2 = 1.0f;
    overlayQuads.push_back(quad);
}

void SecondRender::DrawTestQuads() {
    // Первый квадрат слева: 100x100 пикселей, белый
    DrawOverlayQuad(50.0f, 50.0f, 150.0f, 150.0f, 1.0f, 1.0f, 1.0f);
    
    // Второй квадрат справа: 100x100 пикселей, белый
    DrawOverlayQuad(screenW - 150.0f, 50.0f, screenW - 50.0f, 150.0f, 1.0f, 1.0f, 1.0f);
    
    // Тестовый квадрат в фоне (серый, чтобы проверить слой)
    DrawBackgroundQuad(200.0f, 200.0f, 300.0f, 300.0f, 0.3f, 0.3f, 0.4f);
}

void SecondRender::DrawGrid(int cellSize, int gridSize) {
    if (!backgroundEnabled || !gridConfig.enabled) return;
    
    gridConfig.cellSize = cellSize;
    gridConfig.gridSize = gridSize;
    DrawGridInternal();
}

void SecondRender::SetGridConfig(const GridConfig& config) {
    gridConfig = config;
}

void SecondRender::DrawGridInternal() {
    if (!vulkan) return;
    
    int halfGrid = gridConfig.gridSize / 2;
    int startX = (screenW / 2) - (halfGrid * gridConfig.cellSize);
    int startY = (screenH / 2) - (halfGrid * gridConfig.cellSize);
    
    // Вертикальные линии
    for (int i = 0; i <= gridConfig.gridSize; i++) {
        int x = startX + i * gridConfig.cellSize;
        bool isCenter = (i == gridConfig.gridSize / 2);
        
        float r = isCenter ? gridConfig.centerLineColor[0] : gridConfig.lineColor[0];
        float g = isCenter ? gridConfig.centerLineColor[1] : gridConfig.lineColor[1];
        float b = isCenter ? gridConfig.centerLineColor[2] : gridConfig.lineColor[2];
        
        vulkan->drawQuad(x, startY, x + 1, startY + gridConfig.gridSize * gridConfig.cellSize, r, g, b);
    }
    
    // Горизонтальные линии
    for (int i = 0; i <= gridConfig.gridSize; i++) {
        int y = startY + i * gridConfig.cellSize;
        bool isCenter = (i == gridConfig.gridSize / 2);
        
        float r = isCenter ? gridConfig.centerLineColor[0] : gridConfig.lineColor[0];
        float g = isCenter ? gridConfig.centerLineColor[1] : gridConfig.lineColor[1];
        float b = isCenter ? gridConfig.centerLineColor[2] : gridConfig.lineColor[2];
        
        vulkan->drawQuad(startX, y, startX + gridConfig.gridSize * gridConfig.cellSize, y + 1, r, g, b);
    }
}

void SecondRender::RenderQuads(const std::vector<Quad2D>& quads, int layer) {
    if (!vulkan) return;
    
    for (const auto& quad : quads) {
        if (quad.layer != layer) continue;
        
        if (quad.useTexture && quad.textureId) {
            vulkan->drawImageUV(quad.x1, quad.y1, quad.x2, quad.y2, 
                                (VulkanTexture*)quad.textureId,
                                quad.u1, quad.v1, quad.u2, quad.v2);
        } else {
            vulkan->drawQuad(quad.x1, quad.y1, quad.x2, quad.y2, quad.r, quad.g, quad.b);
        }
    }
}

void SecondRender::RenderBackground() {
    if (!backgroundEnabled || !vulkan) return;
    
    // Рисуем сетку
    DrawGridInternal();
    
    // Рисуем все фоновые квадраты
    RenderQuads(backgroundQuads, 0);
}

void SecondRender::RenderOverlay() {
    if (!overlayEnabled || !vulkan) return;
    
    // Рисуем все оверлейные квадраты
    RenderQuads(overlayQuads, 1);
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
}