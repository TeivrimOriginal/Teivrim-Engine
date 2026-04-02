#include "Panels.h"
#include <iostream>

Panel::Panel(const std::string& n, int _x, int _y, int _w, int _h, bool is3D)
    : name(n), visible(true), dragging(false), dockSide(-1), is3DView(is3D) {
    b.x = _x; b.y = _y; b.width = _w; b.height = _h;
    minW = is3D ? 200 : 150;
    minH = is3D ? 150 : 100;
    b.update();
}

void Panel::setDock(int side, int sw, int sh) {
    dockSide = side;
    if (side == 0) { b.x = 0; b.y = 0; b.width = 250; b.height = sh; }
    else if (side == 1) { b.x = sw - 250; b.y = 0; b.width = 250; b.height = sh; }
    else if (side == 2) { b.x = 0; b.y = 0; b.width = sw; b.height = 50; }
    else if (side == 3) { b.x = 0; b.y = sh - 100; b.width = sw; b.height = 100; }
    b.update();
}

void Panel::updateDock(int sw, int sh) {
    if (dockSide == 0) { b.height = sh; b.update(); }
    else if (dockSide == 1) { b.x = sw - b.width; b.height = sh; b.update(); }
    else if (dockSide == 2) { b.width = sw; b.update(); }
    else if (dockSide == 3) { b.x = 0; b.width = sw; b.y = sh - b.height; b.update(); }
}

void Panel::addButton(const std::string& text, std::function<void()> cb) {
    buttons.push_back({text, cb});
}

void Panel::addLabel(const std::string& text) {
    labels.push_back(text);
}

bool Panel::contains(int px, int py) const {
    return visible && b.contains(px, py);
}

int Panel::getEdge(int px, int py, int s) const {
    if (!visible) return -1;
    if (b.onLeft(px, py, s)) return 0;
    if (b.onRight(px, py, s)) return 1;
    if (b.onTop(px, py, s)) return 2;
    if (b.onBottom(px, py, s)) return 3;
    if (b.onTitle(px, py, 25)) return 4;
    return -1;
}

bool Panel::closeClicked(int px, int py) const {
    if (!visible) return false;
    int cx = b.right - 20, cy = b.y + 5;
    return px >= cx && px <= cx + 12 && py >= cy && py <= cy + 12;
}

bool Panel::handleClick(int px, int py) {
    int by = b.y + 35;
    for (auto& btn : buttons) {
        int bw = btn.first.length() * 8 + 20;
        int bx = b.x + 10;
        if (px >= bx && px <= bx + bw && py >= by && py <= by + 25) {
            btn.second();
            return true;
        }
        by += 30;
    }
    return false;
}

void Panel::startDrag(int mx, int my, int edge) {
    dragging = true;
    dragType = edge;
    dragX = mx; dragY = my;
    dragW = b.width; dragH = b.height;
    dragX0 = b.x; dragY0 = b.y;
}

void Panel::drag(int mx, int my) {
    if (!dragging) return;
    int dx = mx - dragX, dy = my - dragY;
    int nw = dragW, nh = dragH, nx = dragX0, ny = dragY0;
    
    if (dragType == 0) { nw = dragW - dx; nx = dragX0 + dx; }
    else if (dragType == 1) { nw = dragW + dx; }
    else if (dragType == 2) { nh = dragH - dy; ny = dragY0 + dy; }
    else if (dragType == 3) { nh = dragH + dy; }
    else if (dragType == 4) { nx = dragX0 + dx; ny = dragY0 + dy; }
    
    if (dragType != 4) {
        nw = std::max(minW, nw);
        nh = std::max(minH, nh);
        if (dragType == 0) nx = dragX0 + (dragW - nw);
        if (dragType == 2) ny = dragY0 + (dragH - nh);
    }
    
    b.x = nx; b.y = ny; b.width = nw; b.height = nh;
    b.update();
}

void Panel::stopDrag() { dragging = false; }
bool Panel::isDragging() const { return dragging; }
void Panel::setVisible(bool v) { visible = v; }
bool Panel::isVisible() const { return visible; }
bool Panel::is3D() const { return is3DView; }
void Panel::setMinSize(int mw, int mh) { minW = mw; minH = mh; if (b.width < minW) b.width = minW; if (b.height < minH) b.height = minH; b.update(); }

void Panel::render(RenderUI& r) {
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
    
    r.drawText(b.x + 10, b.y + 8, name, 0.9f, 0.9f, 0.9f);
    r.drawText(cx + 3, cy + 2, "X", 1.0f, 1.0f, 1.0f);
    
    int by = b.y + 35;
    for (auto& btn : buttons) {
        int bw = btn.first.length() * 8 + 20;
        int bx = b.x + 10;
        glBegin(GL_QUADS);
        glColor3f(0.4f, 0.4f, 0.5f);
        glVertex2f(bx, by); glVertex2f(bx + bw, by);
        glVertex2f(bx + bw, by + 25); glVertex2f(bx, by + 25);
        glEnd();
        r.drawText(bx + 10, by + 8, btn.first, 1.0f, 1.0f, 1.0f);
        by += 30;
    }
    
    for (auto& lbl : labels) {
        r.drawText(b.x + 10, by, lbl, 0.8f, 0.8f, 0.8f);
        by += 20;
    }
    
    glBegin(GL_QUADS);
}

PanelManager::PanelManager() : active(nullptr), activeEdge(-1), dragging(false), blockInput(false) {}
PanelManager::~PanelManager() { for (auto p : panels) delete p; }

Panel* PanelManager::add(const std::string& name, int x, int y, int w, int h, bool is3D) {
    Panel* p = new Panel(name, x, y, w, h, is3D);
    panels.push_back(p);
    return p;
}

Panel* PanelManager::get3D() {
    for (auto p : panels) if (p->is3D()) return p;
    return nullptr;
}

Panel* PanelManager::at(int px, int py) {
    for (auto p : panels) if (p->isVisible() && p->contains(px, py)) return p;
    return nullptr;
}

void PanelManager::updateDocks(int sw, int sh) {
    for (auto p : panels) p->updateDock(sw, sh);
}

void PanelManager::onMouseDown(int x, int y) {
    for (auto p : panels) {
        if (!p->isVisible()) continue;
        if (p->closeClicked(x, y)) { p->setVisible(false); return; }
        if (p->handleClick(x, y)) return;
        int edge = p->getEdge(x, y);
        if (edge != -1) {
            active = p;
            activeEdge = edge;
            dragging = true;
            blockInput = true;
            p->startDrag(x, y, edge);
            return;
        }
    }
}

void PanelManager::onMouseMove(int x, int y) {
    if (dragging && active) active->drag(x, y);
}

void PanelManager::onMouseUp(int x, int y) {
    if (dragging && active) {
        active->stopDrag();
        dragging = false;
        active = nullptr;
        blockInput = false;
    }
}

bool PanelManager::isBlockingInput() const { return blockInput; }
bool PanelManager::isDragging() const { return dragging; }

void PanelManager::render(RenderUI& r) {
    for (auto p : panels) {
        if (p->isVisible()) {
            glBegin(GL_QUADS);
            p->render(r);
            glEnd();
        }
    }
}