// SecondRender.h
#ifndef SECOND_RENDER_H
#define SECOND_RENDER_H

#include <windows.h>
#include <vector>
#include <string>
#include <functional>

class Vulkan;
class RenderUI;
class InterfaceManager;

struct Quad2D {
    float x1, y1, x2, y2;
    float r, g, b;
    float u1, v1, u2, v2;
    bool useTexture;
    void* textureId;
    int layer; // 0 = background, 1 = overlay
};

struct GridConfig {
    int cellSize;
    int gridSize;
    float lineColor[3];
    float centerLineColor[3];
    bool enabled;
};

class SecondRender {
public:
    static SecondRender& Instance() {
        static SecondRender instance;
        return instance;
    }
    
    // Инициализация с прямым доступом к Vulkan
    void Initialize(Vulkan* vk, RenderUI* ui, int screenWidth, int screenHeight);
    
    // Управление слоями
    void SetBackgroundEnabled(bool enabled) { backgroundEnabled = enabled; }
    void SetOverlayEnabled(bool enabled) { overlayEnabled = enabled; }
    
    // Рисование квадратов
    void DrawBackgroundQuad(float x1, float y1, float x2, float y2, float r, float g, float b);
    void DrawOverlayQuad(float x1, float y1, float x2, float y2, float r, float g, float b);
    void DrawBackgroundImage(float x1, float y1, float x2, float y2, void* texture);
    void DrawOverlayImage(float x1, float y1, float x2, float y2, void* texture);
    
    // Сетка (Grid) для фона
    void DrawGrid(int cellSize = 50, int gridSize = 20);
    void SetGridConfig(const GridConfig& config);
    
    // Тестовые квадраты
    void DrawTestQuads();
    
    // Основные методы рендера
    void RenderBackground();
    void RenderOverlay();
    
    // Clear
    void ClearBackground();
    void ClearOverlay();
    
    // Обновление размера экрана
    void UpdateScreenSize(int width, int height);
    
private:
    SecondRender();
    ~SecondRender();
    
    Vulkan* vulkan;
    RenderUI* renderUI;
    int screenW, screenH;
    bool initialized;
    bool backgroundEnabled;
    bool overlayEnabled;
    
    std::vector<Quad2D> backgroundQuads;
    std::vector<Quad2D> overlayQuads;
    GridConfig gridConfig;
    
    void RenderQuads(const std::vector<Quad2D>& quads, int layer);
    void DrawGridInternal();
};

#endif