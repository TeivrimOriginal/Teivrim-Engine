#include "Panels.h"
#include <iostream>
#include <sstream>

Panel::Panel(const std::string& n, int _x, int _y, int _w, int _h, bool _3D)
    : name(n), visible(true), collapsed(false), is3D(_3D) {
    r.x = _x; r.y = _y; r.w = _w; r.h = _h;
    r.update();
}

void Panel::addButton(const std::string& text, std::function<void()> cb) {
    buttons.push_back({text, cb});
}

void Panel::addLabel(const std::string& text) {
    labels.push_back(text);
}

void Panel::setPos(int x, int y) { r.x = x; r.y = y; r.update(); }
void Panel::setSize(int w, int h) { r.w = w; r.h = h; r.update(); }
void Panel::setVisible(bool v) { visible = v; }
void Panel::setCollapsed(bool c) { collapsed = c; }

bool Panel::contains(int px, int py) const { return visible && r.contains(px, py); }
bool Panel::onHeader(int px, int py) const { return visible && r.onTitle(px, py, 25); }
bool Panel::onCollapseBtn(int px, int py) const {
    if (!visible) return false;
    int cx = r.right - 60, cy = r.y + 5;
    return px >= cx && px <= cx + 12 && py >= cy && py <= cy + 12;
}
bool Panel::onCloseBtn(int px, int py) const {
    if (!visible) return false;
    int cx = r.right - 20, cy = r.y + 5;
    return px >= cx && px <= cx + 12 && py >= cy && py <= cy + 12;
}
bool Panel::onMenuBtn(int px, int py) const {
    if (!visible) return false;
    int cx = r.right - 80, cy = r.y + 5;
    return px >= cx && px <= cx + 12 && py >= cy && py <= cy + 12;
}

int Panel::getEdge(int px, int py, int s) const {
    if (!visible) return -1;
    if (r.onLeft(px, py, s)) return 0;
    if (r.onRight(px, py, s)) return 1;
    if (r.onTop(px, py, s)) return 2;
    if (r.onBottom(px, py, s)) return 3;
    if (r.onTitle(px, py, 25)) return 4;
    return -1;
}

bool Panel::onClickButton(int px, int py) {
    if (collapsed) return false;
    int by = r.y + 35;
    for (auto& btn : buttons) {
        int bw = btn.first.length() * 8 + 20;
        int bx = r.x + 10;
        if (px >= bx && px <= bx + bw && py >= by && py <= by + 25) {
            btn.second();
            return true;
        }
        by += 30;
    }
    return false;
}

void Panel::render(RenderUI& render) {
    if (!visible) return;
    
    if (!is3D) {
        glColor3f(0.18f, 0.18f, 0.22f);
        glVertex2f(r.x, r.y); glVertex2f(r.right, r.y);
        glVertex2f(r.right, r.bottom); glVertex2f(r.x, r.bottom);
    }
    
    glColor3f(0.28f, 0.28f, 0.35f);
    glVertex2f(r.x, r.y); glVertex2f(r.right, r.y);
    glVertex2f(r.right, r.y + 25); glVertex2f(r.x, r.y + 25);
    
    glColor3f(0.4f, 0.4f, 0.45f);
    glVertex2f(r.x, r.y); glVertex2f(r.right, r.y);
    glVertex2f(r.right, r.y + 1); glVertex2f(r.x, r.y + 1);
    glVertex2f(r.x, r.bottom - 1); glVertex2f(r.right, r.bottom - 1);
    glVertex2f(r.right, r.bottom); glVertex2f(r.x, r.bottom);
    glVertex2f(r.x, r.y); glVertex2f(r.x + 1, r.y);
    glVertex2f(r.x + 1, r.bottom); glVertex2f(r.x, r.bottom);
    glVertex2f(r.right - 1, r.y); glVertex2f(r.right, r.y);
    glVertex2f(r.right, r.bottom); glVertex2f(r.right - 1, r.bottom);
    
    int menuX = r.right - 80, collapseX = r.right - 60, closeX = r.right - 20, cy = r.y + 5;
    
    glColor3f(0.35f, 0.35f, 0.45f);
    glVertex2f(menuX, cy); glVertex2f(menuX + 12, cy);
    glVertex2f(menuX + 12, cy + 12); glVertex2f(menuX, cy + 12);
    glVertex2f(collapseX, cy); glVertex2f(collapseX + 12, cy);
    glVertex2f(collapseX + 12, cy + 12); glVertex2f(collapseX, cy + 12);
    
    glColor3f(0.55f, 0.2f, 0.2f);
    glVertex2f(closeX, cy); glVertex2f(closeX + 12, cy);
    glVertex2f(closeX + 12, cy + 12); glVertex2f(closeX, cy + 12);
    
    glEnd();
    
    render.drawText(r.x + 10, r.y + 8, name, 0.9f, 0.9f, 0.9f);
    render.drawText(menuX + 3, cy + 2, "☰", 0.9f, 0.9f, 0.9f);
    render.drawText(collapseX + 3, cy + 2, collapsed ? "▶" : "▼", 0.9f, 0.9f, 0.9f);
    render.drawText(closeX + 3, cy + 2, "✕", 1.0f, 1.0f, 1.0f);
    
    if (!collapsed) {
        int by = r.y + 35;
        for (auto& btn : buttons) {
            int bw = btn.first.length() * 8 + 20;
            int bx = r.x + 10;
            glBegin(GL_QUADS);
            glColor3f(0.35f, 0.35f, 0.45f);
            glVertex2f(bx, by); glVertex2f(bx + bw, by);
            glVertex2f(bx + bw, by + 24); glVertex2f(bx, by + 24);
            glEnd();
            render.drawText(bx + 8, by + 7, btn.first, 1.0f, 1.0f, 1.0f);
            by += 28;
        }
        for (auto& lbl : labels) {
            render.drawText(r.x + 10, by, lbl, 0.7f, 0.7f, 0.7f);
            by += 18;
        }
    }
    
    glBegin(GL_QUADS);
}

PanelManager::PanelManager() : dragging(nullptr), dragX(0), dragY(0), isDrag(false), isResizing(false), menuOpen(false) {}
PanelManager::~PanelManager() { for (auto p : panels) delete p; }

Panel* PanelManager::add(const std::string& name, int x, int y, int w, int h, bool is3D) {
    Panel* p = new Panel(name, x, y, w, h, is3D);
    panels.push_back(p);
    return p;
}

Panel* PanelManager::get3D() {
    for (auto p : panels) if (p->is3D && p->visible) return p;
    return nullptr;
}

Panel* PanelManager::at(int px, int py) {
    for (auto p : panels) if (p->visible && p->contains(px, py)) return p;
    return nullptr;
}

void PanelManager::update(int sw, int sh) {
    // Убираем перезапись размеров. Панели теперь сами хранят свои размеры.
    // Просто обновляем позиции привязанных панелей при изменении окна
    for (auto p : panels) {
        if (p->name == "TopBar") {
            p->setPos(0, 0);
            p->setSize(sw, p->getH()); // сохраняем высоту
        } else if (p->name == "Hierarchy") {
            p->setPos(0, 40);
            p->setSize(p->getW(), sh - 40); // сохраняем ширину
        } else if (p->name == "Inspector") {
            p->setPos(sw - p->getW(), 40);
            p->setSize(p->getW(), sh - 40);
        } else if (p->name == "3D Viewport") {
            // Находим соседей чтобы вычислить ширину
            int leftW = 0, rightW = 0;
            for (auto other : panels) {
                if (other->name == "Hierarchy") leftW = other->getW();
                if (other->name == "Inspector") rightW = other->getW();
            }
            p->setPos(leftW, 40);
            p->setSize(sw - leftW - rightW, sh - 40);
        } else if (p->name == "Console") {
            p->setPos(220, sh - p->getH());
            p->setSize(sw - 220, p->getH());
        }
    }
}

void PanelManager::closeMenu() {
    menuOpen = false;
    menuItems.clear();
    menuCallback = nullptr;
}

void PanelManager::onMouseDown(int x, int y) {
    std::cout << "\n========== MOUSE DOWN ==========" << std::endl;
    std::cout << "Position: " << x << "," << y << std::endl;
    closeMenu();
    
    for (auto p : panels) {
        if (!p->visible) continue;
        
        int edge = p->getEdge(x, y, 8);
        if (edge != -1 && edge != 4) {
            std::cout << ">>> EDGE DETECTED! Panel: " << p->name << " Edge: " << edge << std::endl;
            dragging = p;
            dragX = x;
            dragY = y;
            dragW = p->getW();
            dragH = p->getH();
            dragEdge = edge;
            isDrag = true;
            isResizing = true;
            SetCapture(GetForegroundWindow());
            return;
        }
        
        if (p->onCloseBtn(x, y)) {
            std::cout << "Close button on: " << p->name << std::endl;
            p->visible = false;
            return;
        }
        if (p->onCollapseBtn(x, y)) {
            std::cout << "Collapse button on: " << p->name << std::endl;
            p->collapsed = !p->collapsed;
            return;
        }
        if (p->onMenuBtn(x, y)) {
            std::cout << "Menu button on: " << p->name << std::endl;
            menuOpen = true;
            menuX = x;
            menuY = y + 15;
            menuItems = {"Close", "Collapse", "Reset"};
            menuCallback = [this, p](int idx) {
                if (idx == 0) p->visible = false;
                else if (idx == 1) p->collapsed = !p->collapsed;
                else if (idx == 2) { p->setPos(220, 40); p->setSize(400, 300); }
                closeMenu();
            };
            return;
        }
        if (p->onClickButton(x, y)) return;
        if (p->onHeader(x, y)) {
            std::cout << "Header drag on: " << p->name << std::endl;
            dragging = p;
            dragX = x - p->getX();
            dragY = y - p->getY();
            isDrag = true;
            isResizing = false;
            SetCapture(GetForegroundWindow());
            return;
        }
    }
    
    if (menuOpen) {
        int itemH = 20;
        int idx = (y - menuY) / itemH;
        if (idx >= 0 && idx < (int)menuItems.size() && menuCallback) {
            menuCallback(idx);
        }
        closeMenu();
    }
}

void PanelManager::onMouseMove(int x, int y) {
    if (!isDrag || !dragging) return;
    
    if (isResizing) {
        int dx = x - dragX;
        int dy = y - dragY;
        int nx = dragging->getX();
        int ny = dragging->getY();
        int nw = dragW;
        int nh = dragH;
        
        std::cout << "\n--- RESIZING ---" << std::endl;
        std::cout << "Delta: dx=" << dx << " dy=" << dy << std::endl;
        
        if (dragEdge == 0) {
            nw = dragW - dx;
            nx = dragging->getX() + dx;
            std::cout << "Left edge: newW=" << nw << " newX=" << nx << std::endl;
        } else if (dragEdge == 1) {
            nw = dragW + dx;
            std::cout << "Right edge: newW=" << nw << std::endl;
        } else if (dragEdge == 2) {
            nh = dragH - dy;
            ny = dragging->getY() + dy;
            std::cout << "Top edge: newH=" << nh << " newY=" << ny << std::endl;
        } else if (dragEdge == 3) {
            nh = dragH + dy;
            std::cout << "Bottom edge: newH=" << nh << std::endl;
        }
        
        if (nw < 100) nw = 100;
        if (nh < 50) nh = 50;
        
        dragging->setPos(nx, ny);
        dragging->setSize(nw, nh);
        
        std::cout << "AFTER RESIZE: " << dragging->name << " x=" << dragging->getX() 
                  << " y=" << dragging->getY() << " w=" << dragging->getW() 
                  << " h=" << dragging->getH() << std::endl;
        
        // Ищем и двигаем соседей
        for (auto other : panels) {
            if (other == dragging || !other->visible) continue;
            
            // Проверяем соседа справа
            if (dragEdge == 1 && other->r.x == dragging->r.right) {
                int newW = other->getW() - dx;
                int newX = other->getX() + dx;
                std::cout << "Found RIGHT neighbor: " << other->name << " oldX=" << other->getX() 
                          << " oldW=" << other->getW() << " newX=" << newX << " newW=" << newW << std::endl;
                if (newW >= 100) {
                    other->setPos(newX, other->getY());
                    other->setSize(newW, other->getH());
                    std::cout << "  -> MOVED " << other->name << " to x=" << other->getX() 
                              << " w=" << other->getW() << std::endl;
                }
            }
            // Проверяем соседа слева
            else if (dragEdge == 0 && other->r.right == dragging->r.x) {
                int newW = other->getW() + dx;
                std::cout << "Found LEFT neighbor: " << other->name << " oldW=" << other->getW() 
                          << " newW=" << newW << std::endl;
                if (newW >= 100) {
                    other->setSize(newW, other->getH());
                    std::cout << "  -> RESIZED " << other->name << " to w=" << other->getW() << std::endl;
                }
            }
            // Проверяем соседа снизу
            else if (dragEdge == 3 && other->r.y == dragging->r.bottom) {
                int newH = other->getH() - dy;
                int newY = other->getY() + dy;
                std::cout << "Found BOTTOM neighbor: " << other->name << " oldY=" << other->getY() 
                          << " oldH=" << other->getH() << " newY=" << newY << " newH=" << newH << std::endl;
                if (newH >= 50) {
                    other->setPos(other->getX(), newY);
                    other->setSize(other->getW(), newH);
                    std::cout << "  -> MOVED " << other->name << " to y=" << other->getY() 
                              << " h=" << other->getH() << std::endl;
                }
            }
            // Проверяем соседа сверху
            else if (dragEdge == 2 && other->r.bottom == dragging->r.y) {
                int newH = other->getH() + dy;
                std::cout << "Found TOP neighbor: " << other->name << " oldH=" << other->getH() 
                          << " newH=" << newH << std::endl;
                if (newH >= 50) {
                    other->setSize(other->getW(), newH);
                    std::cout << "  -> RESIZED " << other->name << " to h=" << other->getH() << std::endl;
                }
            }
        }
        
        // Выводим ВСЕ панели после изменений
        std::cout << "\n--- AFTER RESIZE ALL PANELS ---" << std::endl;
        for (auto p : panels) {
            if (p->visible) {
                std::cout << p->name << ": x=" << p->getX() << " y=" << p->getY() 
                          << " w=" << p->getW() << " h=" << p->getH() << std::endl;
            }
        }
        
        dragX = x;
        dragY = y;
        dragW = nw;
        dragH = nh;
    } else {
        dragging->setPos(x - dragX, y - dragY);
        std::cout << "Moving " << dragging->name << " to " << dragging->getX() << "," << dragging->getY() << std::endl;
    }
}

void PanelManager::onMouseUp(int x, int y) {
    std::cout << "\n========== MOUSE UP ==========\n" << std::endl;
    isDrag = false;
    dragging = nullptr;
    isResizing = false;
    ReleaseCapture();
}

bool PanelManager::isDragging() const { return isDrag; }

void PanelManager::render(RenderUI& render) {
    for (auto p : panels) {
        if (p->visible) {
            glBegin(GL_QUADS);
            p->render(render);
            glEnd();
        }
    }
    
    if (menuOpen) {
        int itemH = 20;
        int w = 100;
        int h = menuItems.size() * itemH;
        glBegin(GL_QUADS);
        glColor3f(0.15f, 0.15f, 0.18f);
        glVertex2f(menuX, menuY); glVertex2f(menuX + w, menuY);
        glVertex2f(menuX + w, menuY + h); glVertex2f(menuX, menuY + h);
        glColor3f(0.25f, 0.25f, 0.3f);
        for (size_t i = 0; i < menuItems.size(); i++) {
            int yy = menuY + i * itemH;
            glVertex2f(menuX, yy); glVertex2f(menuX + w, yy);
            glVertex2f(menuX + w, yy + itemH); glVertex2f(menuX, yy + itemH);
        }
        glEnd();
        for (size_t i = 0; i < menuItems.size(); i++) {
            render.drawText(menuX + 5, menuY + i * itemH + 5, menuItems[i], 0.9f, 0.9f, 0.9f);
        }
    }
}