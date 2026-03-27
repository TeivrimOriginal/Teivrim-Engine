#include "Panels.h"
#include <algorithm>

Panels::Panels() {}

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
    
    if (sizes.floatingVisible) {
        renderer.drawQuad(sizes.floatingX, sizes.floatingY, 
                          sizes.floatingX + sizes.floatingWidth, 
                          sizes.floatingY + sizes.floatingHeight, 0.25f, 0.25f, 0.3f);
    }
    
    vector<PanelType> allPanels = {PanelType::Left, PanelType::Right, PanelType::Top, PanelType::Bottom};
    if (sizes.floatingVisible) allPanels.push_back(PanelType::Floating);
    
    for (PanelType panel : allPanels) {
        int panelX, panelY, panelW, panelH;
        getPanelBounds(panel, screenWidth, screenHeight, panelX, panelY, panelW, panelH);
        
        int dotX = panelX + panelW - 25;
        int dotY = panelY + 5;
        renderThreeDots(renderer, dotX, dotY);
        
        int activeDotX = panelX + panelW - 45;
        int activeDotY = panelY + 5;
        renderActiveDot(renderer, activeDotX, activeDotY, activePanel == panel);
    }
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
        case PanelType::Floating:
            outX = sizes.floatingX;
            outY = sizes.floatingY;
            outW = sizes.floatingWidth;
            outH = sizes.floatingHeight;
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

void Panels::setFloatingWidth(int width) {
    sizes.floatingWidth = std::max(sizes.minFloatingWidth, std::min(sizes.maxFloatingWidth, width));
}

void Panels::setFloatingHeight(int height) {
    sizes.floatingHeight = std::max(sizes.minFloatingHeight, std::min(sizes.maxFloatingHeight, height));
}

void Panels::setFloatingPosition(int x, int y) {
    sizes.floatingX = x;
    sizes.floatingY = y;
}

void Panels::setFloatingVisible(bool visible) {
    sizes.floatingVisible = visible;
}

void Panels::updateMinSizes(int minLeft, int minRight, int minTop, int minBottom) {
    sizes.minLeftWidth = minLeft;
    sizes.minRightWidth = minRight;
    sizes.minTopHeight = minTop;
    sizes.minBottomHeight = minBottom;
    
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

bool Panels::isOnThreeDots(int x, int y, PanelType panel, int screenWidth, int screenHeight) const {
    int panelX, panelY, panelW, panelH;
    getPanelBounds(panel, screenWidth, screenHeight, panelX, panelY, panelW, panelH);
    
    int dotX = panelX + panelW - 25;
    int dotY = panelY + 5;
    
    return x >= dotX && x <= dotX + 15 && y >= dotY && y <= dotY + 15;
}

bool Panels::isOnActiveDot(int x, int y, PanelType panel, int screenWidth, int screenHeight) const {
    int panelX, panelY, panelW, panelH;
    getPanelBounds(panel, screenWidth, screenHeight, panelX, panelY, panelW, panelH);
    
    int dotX = panelX + panelW - 45;
    int dotY = panelY + 5;
    
    return x >= dotX && x <= dotX + 15 && y >= dotY && y <= dotY + 15;
}

PanelType Panels::getEdgeAt(int x, int y, int screenWidth, int screenHeight) const {
    if (isOnLeftEdge(x, y, screenWidth, screenHeight)) return PanelType::Left;
    if (isOnRightEdge(x, y, screenWidth, screenHeight)) return PanelType::Right;
    if (isOnTopEdge(x, y, screenWidth, screenHeight)) return PanelType::Top;
    if (isOnBottomEdge(x, y, screenWidth, screenHeight)) return PanelType::Bottom;
    return PanelType::None;
}

PanelType Panels::getPanelAt(int x, int y, int screenWidth, int screenHeight) const {
    int outX, outY, outW, outH;
    
    getPanelBounds(PanelType::Left, screenWidth, screenHeight, outX, outY, outW, outH);
    if (x >= outX && x <= outX + outW && y >= outY && y <= outY + outH) return PanelType::Left;
    
    getPanelBounds(PanelType::Right, screenWidth, screenHeight, outX, outY, outW, outH);
    if (x >= outX && x <= outX + outW && y >= outY && y <= outY + outH) return PanelType::Right;
    
    getPanelBounds(PanelType::Top, screenWidth, screenHeight, outX, outY, outW, outH);
    if (x >= outX && x <= outX + outW && y >= outY && y <= outY + outH) return PanelType::Top;
    
    getPanelBounds(PanelType::Bottom, screenWidth, screenHeight, outX, outY, outW, outH);
    if (x >= outX && x <= outX + outW && y >= outY && y <= outY + outH) return PanelType::Bottom;
    
    if (sizes.floatingVisible) {
        getPanelBounds(PanelType::Floating, screenWidth, screenHeight, outX, outY, outW, outH);
        if (x >= outX && x <= outX + outW && y >= outY && y <= outY + outH) return PanelType::Floating;
    }
    
    return PanelType::None;
}

PanelType Panels::getThreeDotsAt(int x, int y, int screenWidth, int screenHeight) const {
    vector<PanelType> panels = {PanelType::Left, PanelType::Right, PanelType::Top, PanelType::Bottom};
    if (sizes.floatingVisible) panels.push_back(PanelType::Floating);
    
    for (PanelType panel : panels) {
        if (isOnThreeDots(x, y, panel, screenWidth, screenHeight)) {
            return panel;
        }
    }
    return PanelType::None;
}

void Panels::resetPanelSize(PanelType panel) {
    switch (panel) {
        case PanelType::Left:
            setLeftWidth(200);
            break;
        case PanelType::Right:
            setRightWidth(200);
            break;
        case PanelType::Top:
            setTopHeight(50);
            break;
        case PanelType::Bottom:
            setBottomHeight(50);
            break;
        case PanelType::Floating:
            setFloatingWidth(300);
            setFloatingHeight(200);
            break;
        default:
            break;
    }
}

void Panels::splitPanel(PanelType panel) {
    switch (panel) {
        case PanelType::Left:
            setLeftWidth(getLeftWidth() / 2);
            break;
        case PanelType::Right:
            setRightWidth(getRightWidth() / 2);
            break;
        case PanelType::Top:
            setTopHeight(getTopHeight() / 2);
            break;
        case PanelType::Bottom:
            setBottomHeight(getBottomHeight() / 2);
            break;
        default:
            break;
    }
}

void Panels::changePanelType(PanelType panel, const string& newType) {
    switch (panel) {
        case PanelType::Left:
            sizes.leftPanelType = newType;
            break;
        case PanelType::Right:
            sizes.rightPanelType = newType;
            break;
        case PanelType::Top:
            sizes.topPanelType = newType;
            break;
        case PanelType::Bottom:
            sizes.bottomPanelType = newType;
            break;
        case PanelType::Floating:
            sizes.floatingPanelType = newType;
            break;
        default:
            break;
    }
}

void Panels::renderThreeDots(RenderUI& renderer, int x, int y) const {
    glColor3f(0.6f, 0.6f, 0.6f);
    glVertex2f(x, y);
    glVertex2f(x + 3, y);
    glVertex2f(x + 3, y + 3);
    glVertex2f(x, y + 3);
    
    glVertex2f(x + 6, y);
    glVertex2f(x + 9, y);
    glVertex2f(x + 9, y + 3);
    glVertex2f(x + 6, y + 3);
    
    glVertex2f(x + 12, y);
    glVertex2f(x + 15, y);
    glVertex2f(x + 15, y + 3);
    glVertex2f(x + 12, y + 3);
}

void Panels::renderActiveDot(RenderUI& renderer, int x, int y, bool isActive) const {
    if (isActive) {
        glColor3f(0.5f, 0.5f, 0.5f);
    } else {
        glColor3f(0.3f, 0.3f, 0.3f);
    }
    glVertex2f(x, y);
    glVertex2f(x + 8, y);
    glVertex2f(x + 8, y + 8);
    glVertex2f(x, y + 8);
}