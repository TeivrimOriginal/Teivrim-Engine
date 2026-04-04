#ifndef PANELS_H
#define PANELS_H

#include <string>
#include <vector>
#include <functional>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
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
    
    struct ButtonInfo {
        std::string name;
        int x, y, w, h;
        float r, g, b;
        std::function<void()> callback;
    };
    
    struct LabelInfo {
        std::string text;
        int x, y;
        int fontSize;
        bool bold;
        float r, g, b;
    };
    
    std::vector<ButtonInfo> buttons;
    std::vector<LabelInfo> labels;
    
    Panel(const std::string& n, int x, int y, int w, int h, bool _3D = false);
    void addButton(const std::string& text, int x, int y, int w, int h, float cr, float cg, float ccol, std::function<void()> callback);
    void addLabel(const std::string& text, int x, int y, int fontSize, bool bold, float cr, float cg, float cb);
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
    int getEdge(int px, int py, int s=10) const;
    
    int getX() const { return r.x; }
    int getY() const { return r.y; }
    int getW() const { return r.w; }
    int getH() const { return r.h; }
    
    void render(RenderUI& render);
    void setCallback(const std::string& btnName, std::function<void()> cb);
};

class PanelManager {
private:
    std::vector<Panel*> panels;
    Panel* dragging;
    Panel* dragPartner;
    int dragX, dragY;
    int dragW1, dragH1, dragW2, dragH2;
    int dragX1, dragY1, dragX2, dragY2;
    int dragEdge;
    bool isDrag, isResizing, isDoubleEdge;
    bool menuOpen;
    int menuX, menuY;
    std::vector<std::string> menuItems;
    std::function<void(int)> menuCallback;
    int screenW, screenH;
    std::map<std::string, std::function<void()>> globalCallbacks;
    
    void closeMenu();
    void loadUIFromJSON(const std::string& filename);
    
public:
    PanelManager();
    ~PanelManager();
    
    Panel* add(const std::string& name, int x, int y, int w, int h, bool is3D = false);
    void remove(const std::string& name);
    Panel* getPanel(const std::string& name);
    Panel* get3D();
    Panel* at(int px, int py);
    const std::vector<Panel*>& getAll() const { return panels; }
    void update(int sw, int sh);
    
    void onMouseDown(int x, int y);
    void onMouseMove(int x, int y);
    void onMouseUp(int x, int y);
    
    void saveLayout(const std::string& filename);
    void loadLayout(const std::string& filename);
    void loadConfig(const std::string& filename);
    
    void registerCallback(const std::string& name, std::function<void()> cb);
    
    bool isDragging() const;
    bool isBlockingInput() const { return isDrag; }
    void render(RenderUI& render);
};

#endif