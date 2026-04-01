#ifndef TESTNEWUISYSTEM_H
#define TESTNEWUISYSTEM_H

#include <vector>
#include <string>
#include <algorithm>
#include <windows.h>
#include "../Core/Render/Win32/RenderUI.h"

struct PanelBounds {
    int x, y, width, height, right, bottom;
    PanelBounds() : x(0), y(0), width(0), height(0), right(0), bottom(0) {}
    void update() { right = x + width; bottom = y + height; }
    bool contains(int px, int py) const { return px >= x && px <= right && py >= y && py <= bottom; }
    bool intersects(const PanelBounds& other) const {
        return !(right <= other.x || bottom <= other.y || x >= other.right || y >= other.bottom);
    }
    bool onLeft(int px, int py, int s = 5) const { return px >= x - s && px <= x + s && py >= y && py <= bottom; }
    bool onRight(int px, int py, int s = 5) const { return px >= right - s && px <= right + s && py >= y && py <= bottom; }
    bool onTop(int px, int py, int s = 5) const { return py >= y - s && py <= y + s && px >= x && px <= right; }
    bool onBottom(int px, int py, int s = 5) const { return py >= bottom - s && py <= bottom + s && px >= x && px <= right; }
    bool onTitle(int px, int py, int h = 25) const { return px >= x && px <= right && py >= y && py <= y + h; }
};

class FlexiblePanel {
private:
    std::string name;
    PanelBounds b;
    int minW, minH, maxW, maxH;
    bool visible, dragging;
    int dragType, dragX, dragY, dragW, dragH, dragPosX, dragPosY;
    int dockSide;
    bool is3DViewport;
    
public:
    FlexiblePanel(const std::string& n, int x, int y, int w, int h, bool is3D = false) 
        : name(n), visible(true), dragging(false), dockSide(-1), is3DViewport(is3D) {
        b.x = x; b.y = y; b.width = w; b.height = h; b.update();
        minW = is3D ? 200 : 100; minH = is3D ? 150 : 80; 
        maxW = 2000; maxH = 2000;
    }
    
    const std::string& getName() const { return name; }
    int getX() const { return b.x; }
    int getY() const { return b.y; }
    int getWidth() const { return b.width; }
    int getHeight() const { return b.height; }
    const PanelBounds& getBounds() const { return b; }
    bool isVisible() const { return visible; }
    bool is3D() const { return is3DViewport; }
    void setVisible(bool v) { visible = v; }
    void setMinSize(int w, int h) { minW = w; minH = h; if (b.width < minW) setSize(minW, b.height); if (b.height < minH) setSize(b.width, minH); }
    void setMaxSize(int w, int h) { maxW = w; maxH = h; if (b.width > maxW) setSize(maxW, b.height); if (b.height > maxH) setSize(b.width, maxH); }
    void setSize(int w, int h) { b.width = std::max(minW, std::min(maxW, w)); b.height = std::max(minH, std::min(maxH, h)); b.update(); }
    void setPosition(int x, int y) { b.x = x; b.y = y; b.update(); }
    
    void setDockSide(int side, int sw, int sh) {
        dockSide = side;
        if (side == 0) { b.x = 0; b.y = 0; b.width = 250; b.height = sh; }
        else if (side == 1) { b.x = sw - 250; b.y = 0; b.width = 250; b.height = sh; }
        else if (side == 2) { b.x = 0; b.y = 0; b.width = sw; b.height = 50; }
        else if (side == 3) { b.x = 0; b.y = sh - 100; b.width = sw; b.height = 100; }
        b.update();
    }
    
    void updateDock(int sw, int sh) {
        if (dockSide == 0) { b.height = sh; b.update(); }
        else if (dockSide == 1) { b.x = sw - b.width; b.height = sh; b.update(); }
        else if (dockSide == 2) { b.width = sw; b.update(); }
        else if (dockSide == 3) { b.x = 0; b.width = sw; b.y = sh - b.height; b.update(); }
    }
    
    int getDockSide() const { return dockSide; }
    bool contains(int x, int y) const { return visible && b.contains(x, y); }
    
    bool checkCollision(const FlexiblePanel* other) const {
        if (!visible || !other->visible || this == other) return false;
        if (dockSide != -1 || other->dockSide != -1) return false;
        return b.intersects(other->getBounds());
    }
    
    void resolveCollision(FlexiblePanel* other) {
        if (!checkCollision(other)) return;
        int dx = 0, dy = 0;
        if (b.right > other->b.x && b.x < other->b.x) dx = (b.right - other->b.x) + 5;
        else if (b.x < other->b.right && b.right > other->b.right) dx = -(other->b.right - b.x) - 5;
        if (b.bottom > other->b.y && b.y < other->b.y) dy = (b.bottom - other->b.y) + 5;
        else if (b.y < other->b.bottom && b.bottom > other->b.bottom) dy = -(other->b.bottom - b.y) - 5;
        if (abs(dx) < abs(dy)) b.x += dx;
        else b.y += dy;
        b.update();
    }
    
    int getEdge(int x, int y, int s = 5) const {
        if (!visible) return -1;
        if (b.onLeft(x, y, s)) return 0;
        if (b.onRight(x, y, s)) return 1;
        if (b.onTop(x, y, s)) return 2;
        if (b.onBottom(x, y, s)) return 3;
        if (b.onTitle(x, y, 25)) return 4;
        return -1;
    }
    
    void startResize(int mx, int my, int edge) {
        dragging = true; dragType = edge;
        dragX = mx; dragY = my;
        dragW = b.width; dragH = b.height;
        dragPosX = b.x; dragPosY = b.y;
    }
    
    void resize(int mx, int my) {
        if (!dragging) return;
        int dx = mx - dragX, dy = my - dragY;
        int nw = dragW, nh = dragH, nx = dragPosX, ny = dragPosY;
        
        if (dragType == 0) { nw = dragW - dx; nx = dragPosX + dx; }
        else if (dragType == 1) { nw = dragW + dx; }
        else if (dragType == 2) { nh = dragH - dy; ny = dragPosY + dy; }
        else if (dragType == 3) { nh = dragH + dy; }
        
        nw = std::max(minW, std::min(maxW, nw));
        nh = std::max(minH, std::min(maxH, nh));
        if (dragType == 0) nx = dragPosX + (dragW - nw);
        if (dragType == 2) ny = dragPosY + (dragH - nh);
        
        b.x = nx; b.y = ny; b.width = nw; b.height = nh; b.update();
    }
    
    void startMove(int mx, int my) {
        dragging = true; dragType = 4;
        dragX = mx; dragY = my;
        dragPosX = b.x; dragPosY = b.y;
    }
    
    void move(int mx, int my) {
        if (!dragging) return;
        b.x = dragPosX + (mx - dragX);
        b.y = dragPosY + (my - dragY);
        b.update();
    }
    
    void stopDrag() { dragging = false; }
    bool isDragging() const { return dragging; }
    
    bool closeClicked(int px, int py) const {
        if (!visible) return false;
        int cx = b.right - 20, cy = b.y + 5;
        return px >= cx && px <= cx + 12 && py >= cy && py <= cy + 12;
    }
    
    void render(RenderUI& r) const {
        if (!visible) return;
        
        glColor3f(0.2f, 0.2f, 0.25f);
        glVertex2f(b.x, b.y); glVertex2f(b.right, b.y);
        glVertex2f(b.right, b.bottom); glVertex2f(b.x, b.bottom);
        
        glColor3f(0.3f, 0.3f, 0.4f);
        glVertex2f(b.x, b.y); glVertex2f(b.right, b.y);
        glVertex2f(b.right, b.y + 25); glVertex2f(b.x, b.y + 25);
        
        glColor3f(0.5f, 0.5f, 0.5f);
        glVertex2f(b.x, b.y); glVertex2f(b.right, b.y);
        glVertex2f(b.right, b.y + 2); glVertex2f(b.x, b.y + 2);
        
        glVertex2f(b.x, b.bottom - 2); glVertex2f(b.right, b.bottom - 2);
        glVertex2f(b.right, b.bottom); glVertex2f(b.x, b.bottom);
        
        glVertex2f(b.x, b.y); glVertex2f(b.x + 2, b.y);
        glVertex2f(b.x + 2, b.bottom); glVertex2f(b.x, b.bottom);
        
        glVertex2f(b.right - 2, b.y); glVertex2f(b.right, b.y);
        glVertex2f(b.right, b.bottom); glVertex2f(b.right - 2, b.bottom);
        
        int cx = b.right - 20, cy = b.y + 5;
        glColor3f(0.6f, 0.2f, 0.2f);
        glVertex2f(cx, cy); glVertex2f(cx + 12, cy);
        glVertex2f(cx + 12, cy + 12); glVertex2f(cx, cy + 12);
        
        glEnd();
        
        if (is3DViewport) {
            r.drawText(b.x + 10, b.y + 8, name + " (3D View)", 0.2f, 0.8f, 0.2f);
        } else {
            r.drawText(b.x + 10, b.y + 8, name, 0.9f, 0.9f, 0.9f);
        }
        r.drawText(cx + 3, cy + 2, "X", 1.0f, 1.0f, 1.0f);
        
        glBegin(GL_QUADS);
    }
};

class PanelManager {
private:
    std::vector<FlexiblePanel*> panels;
    FlexiblePanel* active;
    int activeEdge;
    bool resizing, moving;
    
    void resolveAllCollisions() {
        for (size_t i = 0; i < panels.size(); i++) {
            for (size_t j = i + 1; j < panels.size(); j++) {
                if (panels[i]->getDockSide() == -1 && panels[j]->getDockSide() == -1) {
                    panels[i]->resolveCollision(panels[j]);
                }
            }
        }
    }
    
public:
    PanelManager() : active(nullptr), activeEdge(-1), resizing(false), moving(false) {}
    ~PanelManager() { for (auto p : panels) delete p; }
    
    FlexiblePanel* create(const std::string& name, int x, int y, int w, int h, bool is3D = false) {
        auto p = new FlexiblePanel(name, x, y, w, h, is3D);
        panels.push_back(p);
        return p;
    }
    
    FlexiblePanel* getAt(int x, int y) {
        for (auto p : panels) if (p->isVisible() && p->contains(x, y)) return p;
        return nullptr;
    }
    
    FlexiblePanel* get3DPanel() {
        for (auto p : panels) if (p->is3D()) return p;
        return nullptr;
    }
    
    const std::vector<FlexiblePanel*>& getAll() const { return panels; }
    
    void onMouseDown(int x, int y) {
        for (auto p : panels) {
            if (!p->isVisible()) continue;
            if (p->closeClicked(x, y)) { p->setVisible(false); return; }
            int edge = p->getEdge(x, y);
            if (edge != -1 && edge != 4) {
                active = p; activeEdge = edge; resizing = true;
                p->startResize(x, y, edge); return;
            }
            if (edge == 4) {
                active = p; moving = true;
                p->startMove(x, y); return;
            }
        }
    }
    
    void onMouseMove(int x, int y) {
        if (resizing && active) {
            active->resize(x, y);
            resolveAllCollisions();
        } else if (moving && active) {
            active->move(x, y);
            resolveAllCollisions();
        }
    }
    
    void onMouseUp(int x, int y) {
        if (resizing && active) { active->stopDrag(); resizing = false; active = nullptr; activeEdge = -1; }
        if (moving && active) { active->stopDrag(); moving = false; active = nullptr; }
    }
    
    bool isDragging() const { return resizing || moving; }
    
    void render(RenderUI& r) {
        for (auto p : panels) {
            if (p->isVisible()) {
                glBegin(GL_QUADS);
                p->render(r);
                glEnd();
            }
        }
    }
};

#endif