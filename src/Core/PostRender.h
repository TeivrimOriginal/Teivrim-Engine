#ifndef POST_RENDER_H
#define POST_RENDER_H

#include <vector>
#include <cstdint>

class Vulkan;

class PostRender {
public:
    void Initialize(Vulkan* vulkan, int screenWidth, int screenHeight);
    void UpdateAndPrintResolution(Vulkan* vulkan, int panelX, int panelY, int panelW, int panelH);
    
    // Новый метод - сканирует ID пикселей в матрицу
    void ScanIDMatrix(Vulkan* vulkan, int panelX, int panelY, int panelW, int panelH);
    
    // Геттер для матрицы ID (ширина x высота)
    const std::vector<std::vector<uint32_t>>& GetIDMatrix() const { return m_idMatrix; }
    
private:
    std::vector<std::vector<uint32_t>> m_idMatrix;
};

#endif