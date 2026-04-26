// SecondRender.cpp - ОБНОВЛЕННЫЙ
#include "SecondRender.h"
#include "Vulkan.h"
#include "Render/Win32/RenderUI.h"
#include "../Interface/InterfaceManager.h"
#include "../Interface/Panels.h"

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

void SecondRender::Initialize(Vulkan* vk, RenderUI* ui, InterfaceManager* manager, int screenWidth, int screenHeight) {
    vulkan = vk;
    renderUI = ui;
    uiManager = manager;
    screenW = screenWidth;
    screenH = screenHeight;
    initialized = (vulkan != nullptr);
    
    UpdateViewportRect();
    
    if (initialized) {
        DrawTestQuads();
    }
}

void SecondRender::UpdateViewportRect() {
    if (!uiManager) return;
    
    Panel* view3D = uiManager->getPanelManager()->get3D();
    if (view3D && view3D->visible && !view3D->collapsed) {
        viewportX = view3D->getX();
        // ВНИМАНИЕ: В OpenGL/Vulkan Y координата перевернута!
        // В RenderUI Y = 0 сверху, в Vulkan drawQuad Y = 0 тоже сверху
        viewportY = view3D->getY();
        viewportW = view3D->getW();
        viewportH = view3D->getH();
        
        // Дополнительная проверка на выход за границы экрана
        if (viewportX < 0) viewportX = 0;
        if (viewportY < 0) viewportY = 0;
        if (viewportX + viewportW > screenW) viewportW = screenW - viewportX;
        if (viewportY + viewportH > screenH) viewportH = screenH - viewportY;
    } else {
        // Если нет 3D панели - рисуем на весь экран (fallback)
        viewportX = 0;
        viewportY = 0;
        viewportW = screenW;
        viewportH = screenH;
    }
}

bool SecondRender::IsPointInViewport(float x, float y) const {
    return (x >= viewportX && x <= viewportX + viewportW &&
            y >= viewportY && y <= viewportY + viewportH);
}

void SecondRender::ClipQuadToViewport(Quad2D& quad) {
    // Обрезаем по X
    if (quad.x1 < viewportX) quad.x1 = viewportX;
    if (quad.x2 > viewportX + viewportW) quad.x2 = viewportX + viewportW;
    if (quad.x1 >= quad.x2) {
        quad.x1 = quad.x2 = 0; // Невидимый квадрат
    }
    
    // Обрезаем по Y
    if (quad.y1 < viewportY) quad.y1 = viewportY;
    if (quad.y2 > viewportY + viewportH) quad.y2 = viewportY + viewportH;
    if (quad.y1 >= quad.y2) {
        quad.y1 = quad.y2 = 0;
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
    
    // Смещаем в координаты Viewport и обрезаем
    quad.x1 += viewportX;
    quad.y1 += viewportY;
    quad.x2 += viewportX;
    quad.y2 += viewportY;
    
    ClipQuadToViewport(quad);
    
    if (quad.x1 < quad.x2 && quad.y1 < quad.y2) {
        backgroundQuads.push_back(quad);
    }
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
    
    // Смещаем в координаты Viewport и обрезаем
    quad.x1 += viewportX;
    quad.y1 += viewportY;
    quad.x2 += viewportX;
    quad.y2 += viewportY;
    
    ClipQuadToViewport(quad);
    
    if (quad.x1 < quad.x2 && quad.y1 < quad.y2) {
        overlayQuads.push_back(quad);
    }
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
    
    quad.x1 += viewportX;
    quad.y1 += viewportY;
    quad.x2 += viewportX;
    quad.y2 += viewportY;
    
    ClipQuadToViewport(quad);
    
    if (quad.x1 < quad.x2 && quad.y1 < quad.y2) {
        backgroundQuads.push_back(quad);
    }
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
    
    quad.x1 += viewportX;
    quad.y1 += viewportY;
    quad.x2 += viewportX;
    quad.y2 += viewportY;
    
    ClipQuadToViewport(quad);
    
    if (quad.x1 < quad.x2 && quad.y1 < quad.y2) {
        overlayQuads.push_back(quad);
    }
}


void SecondRender::DrawTestQuads() {
    UpdateViewportRect();
    
    // ✅ КВАДРАТ В СЛОЕ 0 (BACKGROUND) - будет ПОД 3D моделью
    DrawBackgroundQuad(200.0f, 200.0f, 300.0f, 300.0f, 0.3f, 0.3f, 0.4f);  // Серый квадрат под моделью
    
    // ✅ КВАДРАТЫ В СЛОЕ 2 (OVERLAY) - будут НАД 3D моделью
    DrawOverlayQuad(50.0f, 50.0f, 150.0f, 150.0f, 1.0f, 1.0f, 1.0f);      // Белый квадрат слева (ПОВЕРХ)
    DrawOverlayQuad(viewportW - 150.0f, 50.0f, viewportW - 50.0f, 150.0f, 1.0f, 1.0f, 1.0f); // Белый квадрат справа (ПОВЕРХ)
}

void SecondRender::DrawGrid(int cellSize, int gridSize) {
    if (!backgroundEnabled || !gridConfig.enabled) return;
    
    gridConfig.cellSize = cellSize;
    gridConfig.gridSize = gridSize;
}

void SecondRender::SetGridConfig(const GridConfig& config) {
    gridConfig = config;
}

void SecondRender::DrawGridInternal() {
    if (!vulkan) return;
    
    UpdateViewportRect();
    
    int halfGrid = gridConfig.gridSize / 2;
    int startX = viewportX + (viewportW / 2) - (halfGrid * gridConfig.cellSize);
    int startY = viewportY + (viewportH / 2) - (halfGrid * gridConfig.cellSize);
    
    // Обрезаем начало и конец сетки по Viewport
    int endX = startX + gridConfig.gridSize * gridConfig.cellSize;
    int endY = startY + gridConfig.gridSize * gridConfig.cellSize;
    
    if (startX < viewportX) startX = viewportX;
    if (startY < viewportY) startY = viewportY;
    if (endX > viewportX + viewportW) endX = viewportX + viewportW;
    if (endY > viewportY + viewportH) endY = viewportY + viewportH;
    
    // Вертикальные линии
    for (int i = 0; i <= gridConfig.gridSize; i++) {
        int x = startX + i * gridConfig.cellSize;
        if (x < viewportX || x > viewportX + viewportW) continue;
        
        bool isCenter = (i == gridConfig.gridSize / 2);
        
        float r = isCenter ? gridConfig.centerLineColor[0] : gridConfig.lineColor[0];
        float g = isCenter ? gridConfig.centerLineColor[1] : gridConfig.lineColor[1];
        float b = isCenter ? gridConfig.centerLineColor[2] : gridConfig.lineColor[2];
        
        vulkan->drawQuad(x, startY, x + 1, endY, r, g, b);
    }
    
    // Горизонтальные линии
    for (int i = 0; i <= gridConfig.gridSize; i++) {
        int y = startY + i * gridConfig.cellSize;
        if (y < viewportY || y > viewportY + viewportH) continue;
        
        bool isCenter = (i == gridConfig.gridSize / 2);
        
        float r = isCenter ? gridConfig.centerLineColor[0] : gridConfig.lineColor[0];
        float g = isCenter ? gridConfig.centerLineColor[1] : gridConfig.lineColor[1];
        float b = isCenter ? gridConfig.centerLineColor[2] : gridConfig.lineColor[2];
        
        vulkan->drawQuad(startX, y, endX, y + 1, r, g, b);
    }
}

void SecondRender::RenderQuads(const std::vector<Quad2D>& quads, int layer) {
    if (!vulkan) return;
    
    for (const auto& quad : quads) {
        if (quad.layer != layer) continue;
        
        if (quad.x1 >= quad.x2 || quad.y1 >= quad.y2) continue;
        
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
    
    UpdateViewportRect();
    
    // Рисуем сетку
    DrawGridInternal();
    
    // Рисуем все фоновые квадраты
    RenderQuads(backgroundQuads, 0);
}

void SecondRender::RenderOverlay() {
    if (!overlayEnabled || !vulkan) return;
    
    UpdateViewportRect();
    
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
    UpdateViewportRect();
}