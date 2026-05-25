#ifndef POST_RENDER_H
#define POST_RENDER_H

#include <vector>
#include <cstdint>
#include <iostream>

class Vulkan;

class PostRender {
public:
    PostRender() = default;
    ~PostRender() = default;
    
    void Initialize(Vulkan* vulkan, int screenWidth, int screenHeight);
    void UpdateAndPrintResolution(Vulkan* vulkan, int panelX, int panelY, int panelW, int panelH);
    
    // Сканирует ID буфер в матрицу
    void ScanIDMatrix(Vulkan* vulkan);
    
    // Подсвечивает пиксели с определённым ID (читает каждый кадр)
    void HighlightIDPixels(Vulkan* vulkan, uint32_t targetID, int screenWidth, int screenHeight);
    
    // Подсвечивает пиксели из закэшированной матрицы (быстрее)
    void HighlightFromMatrix(Vulkan* vulkan, uint32_t targetID, int screenWidth, int screenHeight);
    
    // Просто рисует ебаный квадрат в центре
    void DrawTestSquare(Vulkan* vulkan, int screenWidth, int screenHeight);
    
    // Геттер для матрицы ID
    const std::vector<std::vector<uint32_t>>& GetIDMatrix() const { return m_idMatrix; }
    
    // Получить ID по координатам
    uint32_t GetIDAt(int x, int y) const;
    
    // Найти все пиксели с определённым ID
    std::vector<std::pair<int, int>> FindPixelsByID(uint32_t targetID) const;
    
private:
    std::vector<std::vector<uint32_t>> m_idMatrix;
    int m_matrixWidth = 0;
    int m_matrixHeight = 0;
    int m_lastHighlightedID = 0;
    bool m_matrixValid = false;
};

#endif