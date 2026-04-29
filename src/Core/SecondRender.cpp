// SecondRender.cpp
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
    initialized = (vulkan != nullptr);
    
    std::cout << "[SecondRender] Initialize called, vulkan=" << (vulkan ? "OK" : "NULL") 
              << ", screen=" << screenW << "x" << screenH << std::endl;
    
    UpdateViewportRect();
    
    if (initialized) {
        testQuadsDirty = true;
        std::cout << "[SecondRender] Ready, test quads will be created on first render" << std::endl;
    }
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
    
    std::cout << "[SecondRender] Viewport rect: " << viewportX << "," << viewportY 
              << " " << viewportW << "x" << viewportH << std::endl;
    
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

void SecondRender::RebuildTestQuadsIfNeeded() {
    if (!testQuadsDirty) return;
    
    testBackgroundQuads.clear();
    testOverlayQuads.clear();
    
    std::cout << "[SecondRender] Rebuilding test quads: viewportW=" << viewportW << ", viewportH=" << viewportH << std::endl;
    
    Quad2D quad;
    
    // Серый квадрат в BACKGROUND слое (ПОД 3D моделью)
    quad.useTexture = false;
    quad.textureId = nullptr;
    quad.layer = 0;
    quad.x1 = 200.0f + viewportX;
    quad.y1 = 200.0f + viewportY;
    quad.x2 = 300.0f + viewportX;
    quad.y2 = 300.0f + viewportY;
    quad.r = 0.3f; quad.g = 0.3f; quad.b = 0.4f;
    testBackgroundQuads.push_back(quad);
    
    // Белые квадраты в OVERLAY слое (ПОВЕРХ 3D модели)
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
    
    std::cout << "[SecondRender] Test quads rebuilt: bg=" << testBackgroundQuads.size() 
              << ", ov=" << testOverlayQuads.size() << std::endl;
    
    testQuadsDirty = false;
}

void SecondRender::DrawTestQuads() {
    testQuadsDirty = true;
    std::cout << "[SecondRender] DrawTestQuads requested, will rebuild on next render" << std::endl;
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
    
    int halfGrid = gridConfig.gridSize / 2;
    int startX = viewportX + (viewportW / 2) - (halfGrid * gridConfig.cellSize);
    int startY = viewportY + (viewportH / 2) - (halfGrid * gridConfig.cellSize);
    
    int endX = startX + gridConfig.gridSize * gridConfig.cellSize;
    int endY = startY + gridConfig.gridSize * gridConfig.cellSize;
    
    for (int i = 0; i <= gridConfig.gridSize; i++) {
        int x = startX + i * gridConfig.cellSize;
        if (x >= viewportX && x <= viewportX + viewportW && x + 1 <= viewportX + viewportW) {
            bool isCenter = (i == gridConfig.gridSize / 2);
            float r = isCenter ? gridConfig.centerLineColor[0] : gridConfig.lineColor[0];
            float g = isCenter ? gridConfig.centerLineColor[1] : gridConfig.lineColor[1];
            float b = isCenter ? gridConfig.centerLineColor[2] : gridConfig.lineColor[2];
            
            DrawBackgroundQuad(x - viewportX, startY - viewportY, (x + 1) - viewportX, endY - viewportY, r, g, b);
        }
    }
    
    for (int i = 0; i <= gridConfig.gridSize; i++) {
        int y = startY + i * gridConfig.cellSize;
        if (y >= viewportY && y <= viewportY + viewportH && y + 1 <= viewportY + viewportH) {
            bool isCenter = (i == gridConfig.gridSize / 2);
            float r = isCenter ? gridConfig.centerLineColor[0] : gridConfig.lineColor[0];
            float g = isCenter ? gridConfig.centerLineColor[1] : gridConfig.lineColor[1];
            float b = isCenter ? gridConfig.centerLineColor[2] : gridConfig.lineColor[2];
            
            DrawBackgroundQuad(startX - viewportX, y - viewportY, endX - viewportX, (y + 1) - viewportY, r, g, b);
        }
    }
}

void SecondRender::RenderBackground() {
    if (!backgroundEnabled || !vulkan) return;
    
    UpdateViewportRect();
    RebuildTestQuadsIfNeeded();
    
    std::vector<Quad2D> quadsToRender;
    
    // Сначала тестовые квады
    quadsToRender.insert(quadsToRender.end(), testBackgroundQuads.begin(), testBackgroundQuads.end());
    // Потом динамические квады
    quadsToRender.insert(quadsToRender.end(), backgroundQuads.begin(), backgroundQuads.end());
    
    std::cout << "[SecondRender] RenderBackground: " << quadsToRender.size() << " quads (test=" 
              << testBackgroundQuads.size() << ", dynamic=" << backgroundQuads.size() << ")" << std::endl;
    
    DrawGridInternal();
    
    for (const auto& quad : quadsToRender) {
        if (quad.useTexture && quad.textureId) {
            vulkan->drawImageUV(quad.x1, quad.y1, quad.x2, quad.y2, 
                                (VulkanTexture*)quad.textureId,
                                quad.u1, quad.v1, quad.u2, quad.v2);
        } else {
            vulkan->drawBackground(quad.x1, quad.y1, quad.x2, quad.y2, quad.r, quad.g, quad.b);
        }
    }
    
    // Динамические квады очищаем после рендера
    backgroundQuads.clear();
}

void SecondRender::RenderOverlay() {
    if (!overlayEnabled || !vulkan) return;
    
    UpdateViewportRect();
    RebuildTestQuadsIfNeeded();
    
    std::vector<Quad2D> quadsToRender;
    
    quadsToRender.insert(quadsToRender.end(), testOverlayQuads.begin(), testOverlayQuads.end());
    quadsToRender.insert(quadsToRender.end(), overlayQuads.begin(), overlayQuads.end());
    
    std::cout << "[SecondRender] RenderOverlay: " << quadsToRender.size() << " quads (test=" 
              << testOverlayQuads.size() << ", dynamic=" << overlayQuads.size() << ")" << std::endl;
    
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
    std::cout << "[SecondRender] ClearBackground: " << backgroundQuads.size() << " dynamic quads cleared" << std::endl;
    backgroundQuads.clear();
}

void SecondRender::ClearOverlay() {
    std::cout << "[SecondRender] ClearOverlay: " << overlayQuads.size() << " dynamic quads cleared" << std::endl;
    overlayQuads.clear();
}

void SecondRender::UpdateScreenSize(int width, int height) {
    screenW = width;
    screenH = height;
    std::cout << "[SecondRender] UpdateScreenSize: " << width << "x" << height << std::endl;
    UpdateViewportRect();
    testQuadsDirty = true;
}
// SecondRender.cpp - добавляем в конец файла, перед закрывающими скобками

void SecondRender::SetCamera(const glm::mat4& viewMatrix, const glm::mat4& projMatrix, const glm::vec3& cameraPos) {
    currentViewMat = viewMatrix;
    currentProjMat = projMatrix;
    currentCameraPos = cameraPos;
    cameraMatrixValid = true;
}

std::vector<GridLine> SecondRender::CalculateGridLines() {
    std::vector<GridLine> lines;
    
    if (!gridConfig.infiniteGrid || !cameraMatrixValid) return lines;
    
    float spacing = gridConfig.gridSpacing;
    float fadeDist = gridConfig.fadeDistance;
    float yOffset = gridConfig.yOffset;
    
    // Получаем frustum плоскостей в мировом пространстве
    glm::mat4 VP = currentProjMat * currentViewMat;
    glm::mat4 invVP = glm::inverse(VP);
    
    // Получаем 8 углов frustum в мировом пространстве
    glm::vec4 frustumCorners[8];
    for (int i = 0; i < 8; i++) {
        float x = (i & 1) ? 1.0f : -1.0f;
        float y = (i & 2) ? 1.0f : -1.0f;
        float z = (i & 4) ? 1.0f : -1.0f;
        glm::vec4 clipSpace(x, y, z, 1.0f);
        glm::vec4 worldSpace = invVP * clipSpace;
        worldSpace /= worldSpace.w;
        frustumCorners[i] = worldSpace;
    }
    
    // Находим min и max X/Z в frustum
    float minX = FLT_MAX, maxX = -FLT_MAX;
    float minZ = FLT_MAX, maxZ = -FLT_MAX;
    
    for (int i = 0; i < 8; i++) {
        minX = std::min(minX, frustumCorners[i].x);
        maxX = std::max(maxX, frustumCorners[i].x);
        minZ = std::min(minZ, frustumCorners[i].z);
        maxZ = std::max(maxZ, frustumCorners[i].z);
    }
    
    // Расширяем границы с учетом расстояния камеры
    float camX = currentCameraPos.x;
    float camZ = currentCameraPos.z;
    float range = std::max(fabsf(maxX - minX), fabsf(maxZ - minZ)) * 1.5f;
    
    minX = std::min(minX, camX - range);
    maxX = std::max(maxX, camX + range);
    minZ = std::min(minZ, camZ - range);
    maxZ = std::max(maxZ, camZ + range);
    
    // Округляем до ближайшего шага грид
    minX = floor(minX / spacing) * spacing;
    maxX = ceil(maxX / spacing) * spacing;
    minZ = floor(minZ / spacing) * spacing;
    maxZ = ceil(maxZ / spacing) * spacing;
    
    // Ограничиваем максимальное количество линий (для производительности)
    int maxLines = 200;
    int xSteps = std::min((int)((maxX - minX) / spacing), maxLines);
    int zSteps = std::min((int)((maxZ - minZ) / spacing), maxLines);
    
    float centerX = floor(camX / spacing) * spacing;
    float centerZ = floor(camZ / spacing) * spacing;
    
    // Линии по X
    for (int i = -xSteps/2; i <= xSteps/2; i++) {
        float x = centerX + i * spacing;
        float dist = fabs(x - camX);
        float alpha = 1.0f - std::min(1.0f, dist / fadeDist);
        
        if (alpha <= 0.05f) continue;
        
        GridLine line;
        line.start = glm::vec3(x, yOffset, minZ);
        line.end = glm::vec3(x, yOffset, maxZ);
        line.alpha = alpha;
        line.isCenter = (fabs(x) < spacing * 0.1f);
        lines.push_back(line);
    }
    
    // Линии по Z
    for (int i = -zSteps/2; i <= zSteps/2; i++) {
        float z = centerZ + i * spacing;
        float dist = fabs(z - camZ);
        float alpha = 1.0f - std::min(1.0f, dist / fadeDist);
        
        if (alpha <= 0.05f) continue;
        
        GridLine line;
        line.start = glm::vec3(minX, yOffset, z);
        line.end = glm::vec3(maxX, yOffset, z);
        line.alpha = alpha;
        line.isCenter = (fabs(z) < spacing * 0.1f);
        lines.push_back(line);
    }
    
    return lines;
}

void SecondRender::DrawInfiniteGridLines(const std::vector<GridLine>& lines) {
    if (!vulkan || lines.empty()) return;
    
    // Сохраняем текущие матрицы
    glm::mat4 oldView = vulkan->getViewMatrix();
    glm::mat4 oldProj = vulkan->getProjectionMatrix();
    
    // Создаем ортографическую проекцию для 2D проецирования линии
    // Но для 3D линий используем обычные матрицы
    // Здесь мы рисуем линии как 3D объекты
    
    for (const auto& line : lines) {
        float r = line.isCenter ? gridConfig.centerLineColor[0] : gridConfig.lineColor[0];
        float g = line.isCenter ? gridConfig.centerLineColor[1] : gridConfig.lineColor[1];
        float b = line.isCenter ? gridConfig.centerLineColor[2] : gridConfig.lineColor[2];
        
        // Проецируем 3D точки на экран
        glm::vec4 clipStart = currentProjMat * currentViewMat * glm::vec4(line.start, 1.0f);
        glm::vec4 clipEnd = currentProjMat * currentViewMat * glm::vec4(line.end, 1.0f);
        
        if (clipStart.w <= 0 || clipEnd.w <= 0) continue;
        
        glm::vec3 ndcStart = glm::vec3(clipStart) / clipStart.w;
        glm::vec3 ndcEnd = glm::vec3(clipEnd) / clipEnd.w;
        
        // Конвертируем NDC в экранные координаты
        float screenX1 = (ndcStart.x + 1.0f) * 0.5f * viewportW + viewportX;
        float screenY1 = (1.0f - ndcStart.y) * 0.5f * viewportH + viewportY;
        float screenX2 = (ndcEnd.x + 1.0f) * 0.5f * viewportW + viewportX;
        float screenY2 = (1.0f - ndcEnd.y) * 0.5f * viewportH + viewportY;
        
        // Рисуем линию как тонкий прямоугольник
        float dx = screenX2 - screenX1;
        float dy = screenY2 - screenY1;
        float len = sqrt(dx*dx + dy*dy);
        
        if (len < 0.001f) continue;
        
        float thickness = 1.0f;
        float perpX = -dy / len * thickness;
        float perpY = dx / len * thickness;
        
        float alpha = line.alpha;
        
        // Рисуем линию как два треугольника
        std::vector<Quad2D> lineQuads;
        
        // Временно переключаемся на overlay слой для линий (чтобы были поверх грид)
        DrawOverlayQuad(screenX1 + perpX, screenY1 + perpY, screenX2 + perpX, screenY2 + perpY, r*alpha, g*alpha, b*alpha);
        DrawOverlayQuad(screenX1 - perpX, screenY1 - perpY, screenX2 - perpX, screenY2 - perpY, r*alpha, g*alpha, b*alpha);
        DrawOverlayQuad(screenX1 + perpX, screenY1 + perpY, screenX2 - perpX, screenY2 - perpY, r*alpha, g*alpha, b*alpha);
        DrawOverlayQuad(screenX1 - perpX, screenY1 - perpY, screenX2 + perpX, screenY2 + perpY, r*alpha, g*alpha, b*alpha);
    }
}

void SecondRender::RenderInfiniteGrid() {
    if (!gridConfig.enabled || !gridConfig.infiniteGrid || !vulkan) return;
    
    auto lines = CalculateGridLines();
    DrawInfiniteGridLines(lines);
}