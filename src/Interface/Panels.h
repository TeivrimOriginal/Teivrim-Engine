#ifndef PANELS_H
#define PANELS_H
#include <string>
#include <vector>
#include "../Core/Render/Win32/RenderUI.h"

using namespace std;

enum class PanelType {
    None,
    Left,
    Right,
    Top,
    Bottom,
    Floating
};

struct PanelSizes {
    int leftWidth = 200;
    int rightWidth = 200;
    int topHeight = 50;
    int bottomHeight = 50;
    int floatingWidth = 300;
    int floatingHeight = 200;
    
    int minLeftWidth = 50;
    int minRightWidth = 50;
    int minTopHeight = 30;
    int minBottomHeight = 30;
    int minFloatingWidth = 150;
    int minFloatingHeight = 100;
    
    int maxLeftWidth = 400;
    int maxRightWidth = 400;
    int maxTopHeight = 200;
    int maxBottomHeight = 200;
    int maxFloatingWidth = 800;
    int maxFloatingHeight = 600;
    
    string leftPanelType = "Hierarchy";
    string rightPanelType = "Inspector";
    string topPanelType = "Scene Control";
    string bottomPanelType = "Asset Manager";
    string floatingPanelType = "Floating Panel";
    
    int floatingX = 100;
    int floatingY = 100;
    bool floatingVisible = false;
};

struct PanelDimensions {
    int leftPanelWidth;
    int rightPanelWidth;
    int topPanelHeight;
    int bottomPanelHeight;
    int centerX;
    int centerY;
    int centerWidth;
    int centerHeight;
};

class Panels {
private:
    PanelSizes sizes;
    PanelType activePanel = PanelType::None;
    
public:
    Panels();
    
    PanelDimensions getDimensions(int screenWidth, int screenHeight) const;
    void render(RenderUI& renderer, int screenWidth, int screenHeight) const;
    void getPanelBounds(PanelType panel, int screenWidth, int screenHeight, 
                        int& outX, int& outY, int& outW, int& outH) const;
    
    void setLeftWidth(int width);
    void setRightWidth(int width);
    void setTopHeight(int height);
    void setBottomHeight(int height);
    void setFloatingWidth(int width);
    void setFloatingHeight(int height);
    void setFloatingPosition(int x, int y);
    void setFloatingVisible(bool visible);
    
    void updateMinSizes(int minLeft, int minRight, int minTop, int minBottom);
    
    int getLeftWidth() const { return sizes.leftWidth; }
    int getRightWidth() const { return sizes.rightWidth; }
    int getTopHeight() const { return sizes.topHeight; }
    int getBottomHeight() const { return sizes.bottomHeight; }
    int getFloatingWidth() const { return sizes.floatingWidth; }
    int getFloatingHeight() const { return sizes.floatingHeight; }
    int getFloatingX() const { return sizes.floatingX; }
    int getFloatingY() const { return sizes.floatingY; }
    bool getFloatingVisible() const { return sizes.floatingVisible; }
    
    string getLeftPanelType() const { return sizes.leftPanelType; }
    string getRightPanelType() const { return sizes.rightPanelType; }
    string getTopPanelType() const { return sizes.topPanelType; }
    string getBottomPanelType() const { return sizes.bottomPanelType; }
    string getFloatingPanelType() const { return sizes.floatingPanelType; }
    
    void setLeftPanelType(const string& type) { sizes.leftPanelType = type; }
    void setRightPanelType(const string& type) { sizes.rightPanelType = type; }
    void setTopPanelType(const string& type) { sizes.topPanelType = type; }
    void setBottomPanelType(const string& type) { sizes.bottomPanelType = type; }
    void setFloatingPanelType(const string& type) { sizes.floatingPanelType = type; }
    
    void setActivePanel(PanelType panel) { activePanel = panel; }
    PanelType getActivePanel() const { return activePanel; }
    
    bool isOnLeftEdge(int x, int y, int screenWidth, int screenHeight) const;
    bool isOnRightEdge(int x, int y, int screenWidth, int screenHeight) const;
    bool isOnTopEdge(int x, int y, int screenWidth, int screenHeight) const;
    bool isOnBottomEdge(int x, int y, int screenWidth, int screenHeight) const;
    bool isOnThreeDots(int x, int y, PanelType panel, int screenWidth, int screenHeight) const;
    bool isOnActiveDot(int x, int y, PanelType panel, int screenWidth, int screenHeight) const;
    
    PanelType getEdgeAt(int x, int y, int screenWidth, int screenHeight) const;
    PanelType getPanelAt(int x, int y, int screenWidth, int screenHeight) const;
    PanelType getThreeDotsAt(int x, int y, int screenWidth, int screenHeight) const;
    
    void resetPanelSize(PanelType panel);
    void splitPanel(PanelType panel);
    void changePanelType(PanelType panel, const string& newType);
    
private:
    void renderThreeDots(RenderUI& renderer, int x, int y) const;
    void renderActiveDot(RenderUI& renderer, int x, int y, bool isActive) const;
};

#endif