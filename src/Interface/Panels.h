#ifndef PANELS_H
#define PANELS_H

#include <string>
#include <vector>
#include <functional>
#include <windows.h>
#include "../Core/Render/Win32/RenderUI.h"

struct Rect {
    int x, y, w, h, right, bottom;
    Rect() : x(0), y(0), w(0), h(0), right(0), bottom(0) {}
    void update() { right = x + w; bottom = y + h; }
    bool contains(int px, int py) const { return px >= x && px <= right && py >= y && py <= bottom; }
    bool onLeft(int px, int py, int s=5) const { return px >= x-s && px <= x+s && py >= y && py <= bottom; }
    bool onRight(int px, int py, int s=5) const { return px >= right-s && px <= right+s && py >= y && py <= bottom; }
    bool onTop(int px, int py, int s=5) const { return py >= y-s && py <= y+s && px >= x && px <= right; }
    bool onBottom(int px, int py, int s=5) const { return py >= bottom-s && py <= bottom+s && px >= x && px <= right; }
    bool onTitle(int px, int py, int h=25) const { return px >= x && px <= right && py >= y && py <= y+h; }
};

class Panel {
public:
    std::string name;
    Rect r;
    bool visible, collapsed;
    bool is3D;
    std::vector<std::pair<std::string, std::function<void()>>> buttons;
    std::vector<std::string> labels;
    
    Panel(const std::string& n, int x, int y, int w, int h, bool _3D = false);
    void addButton(const std::string& text, std::function<void()> cb);
    void addLabel(const std::string& text);
    void setPos(int x, int y);
    void setSize(int w, int h);
    void setVisible(bool v);
    void setCollapsed(bool c);
    
    bool contains(int px, int py) const;
    bool onHeader(int px, int py) const;
    bool onCollapseBtn(int px, int py) const;
    bool onCloseBtn(int px, int py) const;
    bool onMenuBtn(int px, int py) const;
    bool onClickButton(int px, int py);
    int getEdge(int px, int py, int s=5) const;
    
    int getX() const { return r.x; }
    int getY() const { return r.y; }
    int getW() const { return r.w; }
    int getH() const { return r.h; }
    
    void render(RenderUI& render);
};

class PanelManager {
private:
    std::vector<Panel*> panels;
    Panel* dragging;
    int dragX, dragY, dragW, dragH, dragEdge;
    bool isDrag, isResizing;
    bool menuOpen;
    int menuX, menuY;
    std::vector<std::string> menuItems;
    std::function<void(int)> menuCallback;
    
    void closeMenu();
    
public:
    PanelManager();
    ~PanelManager();
    
    Panel* add(const std::string& name, int x, int y, int w, int h, bool is3D = false);
    Panel* get3D();
    Panel* at(int px, int py);
    void update(int sw, int sh);
    
    void onMouseDown(int x, int y);
    void onMouseMove(int x, int y);
    void onMouseUp(int x, int y);
    
    bool isDragging() const;
    bool isBlockingInput() const { return isDrag; }
    void render(RenderUI& render);
};

#endif