#ifndef PANELS_H
#define PANELS_H

#include <string>
#include <vector>
#include <functional>
#include <windows.h>
#include "../Core/Render/Win32/RenderUI.h"

struct PanelBounds {
    int x, y, width, height, right, bottom;
    PanelBounds() : x(0), y(0), width(0), height(0), right(0), bottom(0) {}
    void update() { right = x + width; bottom = y + height; }
    bool contains(int px, int py) const { return px >= x && px <= right && py >= y && py <= bottom; }
    bool onLeft(int px, int py, int s=5) const { return px >= x-s && px <= x+s && py >= y && py <= bottom; }
    bool onRight(int px, int py, int s=5) const { return px >= right-s && px <= right+s && py >= y && py <= bottom; }
    bool onTop(int px, int py, int s=5) const { return py >= y-s && py <= y+s && px >= x && px <= right; }
    bool onBottom(int px, int py, int s=5) const { return py >= bottom-s && py <= bottom+s && px >= x && px <= right; }
    bool onTitle(int px, int py, int h=25) const { return px >= x && px <= right && py >= y && py <= y+h; }
};

class Panel {
private:
    std::string name;
    PanelBounds b;
    int minW, minH;
    bool visible, dragging;
    int dragType, dragX, dragY, dragW, dragH, dragX0, dragY0;
    int dockSide;
    bool is3DView;
    std::vector<std::pair<std::string, std::function<void()>>> buttons;
    std::vector<std::string> labels;
    
public:
    Panel(const std::string& n, int _x, int _y, int _w, int _h, bool is3D = false);
    
    void setDock(int side, int sw, int sh);
    void updateDock(int sw, int sh);
    void addButton(const std::string& text, std::function<void()> cb);
    void addLabel(const std::string& text);
    
    bool contains(int px, int py) const;
    int getEdge(int px, int py, int s=5) const;
    bool closeClicked(int px, int py) const;
    bool handleClick(int px, int py);
    
    void startDrag(int mx, int my, int edge);
    void drag(int mx, int my);
    void stopDrag();
    bool isDragging() const;
    
    void setVisible(bool v);
    bool isVisible() const;
    bool is3D() const;
    void setMinSize(int mw, int mh);
    int getX() const { return b.x; }
    int getY() const { return b.y; }
    int getW() const { return b.width; }
    int getH() const { return b.height; }
    
    void render(RenderUI& r);
};

class PanelManager {
private:
    std::vector<Panel*> panels;
    Panel* active;
    int activeEdge;
    bool dragging;
    bool blockInput;
    
public:
    PanelManager();
    ~PanelManager();
    
    Panel* add(const std::string& name, int x, int y, int w, int h, bool is3D = false);
    Panel* get3D();
    Panel* at(int px, int py);
    void updateDocks(int sw, int sh);
    
    void onMouseDown(int x, int y);
    void onMouseMove(int x, int y);
    void onMouseUp(int x, int y);
    
    bool isBlockingInput() const;
    bool isDragging() const;
    void render(RenderUI& r);
};

#endif