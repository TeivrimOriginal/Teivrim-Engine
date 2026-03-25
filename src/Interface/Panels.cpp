#include "Panels.h"
#include <algorithm>

Panels::Panels() {
    // Инициализация по умолчанию
}

PanelDimensions Panels::getDimensions(int screenWidth, int screenHeight) const {
    PanelDimensions dims;
    
    dims.leftPanelWidth = sizes.leftWidth;
    dims.rightPanelWidth = sizes.rightWidth;
    dims.topPanelHeight = sizes.topHeight;
    dims.bottomPanelHeight = sizes.bottomHeight;
    
    dims.centerX = dims.leftPanelWidth;
    dims.centerY = dims.topPanelHeight;
    dims.centerWidth = screenWidth - (dims.leftPanelWidth + dims.rightPanelWidth);
    dims.centerHeight = screenHeight - (dims.topPanelHeight + dims.bottomPanelHeight);
    
    return dims;
}

void Panels::render(RenderUI& renderer, int screenWidth, int screenHeight) const {
    PanelDimensions dims = getDimensions(screenWidth, screenHeight);
    
    renderer.drawQuad(0, 0, dims.leftPanelWidth, screenHeight, 0.2f, 0.2f, 0.2f);
    renderer.drawQuad(screenWidth - dims.rightPanelWidth, 0, screenWidth, screenHeight, 0.2f, 0.2f, 0.2f);
    renderer.drawQuad(dims.leftPanelWidth, 0, screenWidth - dims.rightPanelWidth, dims.topPanelHeight, 0.3f, 0.3f, 0.3f);
    renderer.drawQuad(dims.leftPanelWidth, screenHeight - dims.bottomPanelHeight, 
                      screenWidth - dims.rightPanelWidth, screenHeight, 0.3f, 0.3f, 0.3f);
}

void Panels::getPanelBounds(PanelType panel, int screenWidth, int screenHeight, 
                            int& outX, int& outY, int& outW, int& outH) const {
    PanelDimensions dims = getDimensions(screenWidth, screenHeight);
    
    switch (panel) {
        case PanelType::Left:
            outX = 0;
            outY = dims.topPanelHeight;
            outW = dims.leftPanelWidth;
            outH = screenHeight - dims.topPanelHeight - dims.bottomPanelHeight;
            break;
        case PanelType::Right:
            outX = screenWidth - dims.rightPanelWidth;
            outY = dims.topPanelHeight;
            outW = dims.rightPanelWidth;
            outH = screenHeight - dims.topPanelHeight - dims.bottomPanelHeight;
            break;
        case PanelType::Top:
            outX = dims.leftPanelWidth;
            outY = 0;
            outW = screenWidth - dims.leftPanelWidth - dims.rightPanelWidth;
            outH = dims.topPanelHeight;
            break;
        case PanelType::Bottom:
            outX = dims.leftPanelWidth;
            outY = screenHeight - dims.bottomPanelHeight;
            outW = screenWidth - dims.leftPanelWidth - dims.rightPanelWidth;
            outH = dims.bottomPanelHeight;
            break;
        default:
            outX = outY = outW = outH = 0;
            break;
    }
}

void Panels::setLeftWidth(int width) {
    sizes.leftWidth = std::max(sizes.minLeftWidth, std::min(sizes.maxLeftWidth, width));
}

void Panels::setRightWidth(int width) {
    sizes.rightWidth = std::max(sizes.minRightWidth, std::min(sizes.maxRightWidth, width));
}

void Panels::setTopHeight(int height) {
    sizes.topHeight = std::max(sizes.minTopHeight, std::min(sizes.maxTopHeight, height));
}

void Panels::setBottomHeight(int height) {
    sizes.bottomHeight = std::max(sizes.minBottomHeight, std::min(sizes.maxBottomHeight, height));
}

void Panels::updateMinSizes(int minLeft, int minRight, int minTop, int minBottom) {
    sizes.minLeftWidth = minLeft;
    sizes.minRightWidth = minRight;
    sizes.minTopHeight = minTop;
    sizes.minBottomHeight = minBottom;
    
    // Применяем новые минимумы к текущим размерам, если они меньше минимума
    if (sizes.leftWidth < sizes.minLeftWidth) setLeftWidth(sizes.minLeftWidth);
    if (sizes.rightWidth < sizes.minRightWidth) setRightWidth(sizes.minRightWidth);
    if (sizes.topHeight < sizes.minTopHeight) setTopHeight(sizes.minTopHeight);
    if (sizes.bottomHeight < sizes.minBottomHeight) setBottomHeight(sizes.minBottomHeight);
}

bool Panels::isOnLeftEdge(int x, int y, int screenWidth, int screenHeight) const {
    int edgeX = sizes.leftWidth;
    int zoneMin = edgeX - 3;
    int zoneMax = edgeX + 3;
    
    if (x < zoneMin || x > zoneMax) return false;
    return y >= 0 && y <= screenHeight;
}

bool Panels::isOnRightEdge(int x, int y, int screenWidth, int screenHeight) const {
    int edgeX = screenWidth - sizes.rightWidth;
    int zoneMin = edgeX - 3;
    int zoneMax = edgeX + 3;
    
    if (x < zoneMin || x > zoneMax) return false;
    return y >= 0 && y <= screenHeight;
}

bool Panels::isOnTopEdge(int x, int y, int screenWidth, int screenHeight) const {
    int edgeY = sizes.topHeight;
    int zoneMin = edgeY - 3;
    int zoneMax = edgeY + 3;
    
    if (y < zoneMin || y > zoneMax) return false;
    return x >= sizes.leftWidth && x <= (screenWidth - sizes.rightWidth);
}

bool Panels::isOnBottomEdge(int x, int y, int screenWidth, int screenHeight) const {
    int edgeY = screenHeight - sizes.bottomHeight;
    int zoneMin = edgeY - 3;
    int zoneMax = edgeY + 3;
    
    if (y < zoneMin || y > zoneMax) return false;
    return x >= sizes.leftWidth && x <= (screenWidth - sizes.rightWidth);
}

PanelType Panels::getEdgeAt(int x, int y, int screenWidth, int screenHeight) const {
    if (isOnLeftEdge(x, y, screenWidth, screenHeight)) return PanelType::Left;
    if (isOnRightEdge(x, y, screenWidth, screenHeight)) return PanelType::Right;
    if (isOnTopEdge(x, y, screenWidth, screenHeight)) return PanelType::Top;
    if (isOnBottomEdge(x, y, screenWidth, screenHeight)) return PanelType::Bottom;
    return PanelType::None;
}