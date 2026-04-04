#include "Panels.h"
#include <sstream>

Panel::Panel(const std::string& n, int _x, int _y, int _w, int _h, bool _3D)
    : name(n), visible(true), collapsed(false), is3D(_3D) {
    r.x = _x; r.y = _y; r.w = _w; r.h = _h;
    r.update();
}

void Panel::addButton(const std::string& text, int _x, int _y, int _w, int _h, float cr, float cg, float ccol, std::function<void()> callback) {
    ButtonInfo btn;
    btn.name = text;
    btn.x = _x; btn.y = _y; btn.w = _w; btn.h = _h;
    btn.r = cr; btn.g = cg; btn.b = ccol;
    btn.callback = callback;
    buttons.push_back(btn);
}

void Panel::addLabel(const std::string& text, int _x, int _y, int fontSize, bool bold, float cr, float cg, float cb) {
    LabelInfo lbl;
    lbl.text = text;
    lbl.x = _x; lbl.y = _y;
    lbl.fontSize = fontSize;
    lbl.bold = bold;
    lbl.r = cr; lbl.g = cg; lbl.b = cb;
    labels.push_back(lbl);
}

void Panel::setCallback(const std::string& btnName, std::function<void()> cb) {
    for (auto& btn : buttons) {
        if (btn.name == btnName) {
            btn.callback = cb;
            break;
        }
    }
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
    for (auto& btn : buttons) {
        int bx = r.x + btn.x;
        int by = r.y + btn.y;
        if (px >= bx && px <= bx + btn.w && py >= by && py <= by + btn.h) {
            if (btn.callback) btn.callback();
            return true;
        }
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
        for (auto& btn : buttons) {
            int bx = r.x + btn.x;
            int by = r.y + btn.y;
            glBegin(GL_QUADS);
            glColor3f(btn.r, btn.g, btn.b);
            glVertex2f(bx, by); glVertex2f(bx + btn.w, by);
            glVertex2f(bx + btn.w, by + btn.h); glVertex2f(bx, by + btn.h);
            glEnd();
            render.drawText(bx + 8, by + 7, btn.name, 1.0f, 1.0f, 1.0f);
        }
        for (auto& lbl : labels) {
            render.drawText(r.x + lbl.x, r.y + lbl.y, lbl.text, lbl.r, lbl.g, lbl.b);
        }
    }
    
    glBegin(GL_QUADS);
}

PanelManager::PanelManager() : dragging(nullptr), dragPartner(nullptr), dragX(0), dragY(0), 
    dragW1(0), dragH1(0), dragW2(0), dragH2(0), dragX1(0), dragY1(0), dragX2(0), dragY2(0),
    dragEdge(-1), isDrag(false), isResizing(false), isDoubleEdge(false), menuOpen(false), screenW(1280), screenH(720) {}
PanelManager::~PanelManager() { for (auto p : panels) delete p; }

Panel* PanelManager::add(const std::string& name, int x, int y, int w, int h, bool is3D) {
    Panel* p = new Panel(name, x, y, w, h, is3D);
    panels.push_back(p);
    return p;
}

void PanelManager::remove(const std::string& name) {
    for (auto it = panels.begin(); it != panels.end(); ++it) {
        if ((*it)->name == name) {
            delete *it;
            panels.erase(it);
            break;
        }
    }
}

Panel* PanelManager::getPanel(const std::string& name) {
    for (auto p : panels) {
        if (p->name == name) return p;
    }
    return nullptr;
}

Panel* PanelManager::get3D() {
    for (auto p : panels) if (p->is3D && p->visible) return p;
    return nullptr;
}

Panel* PanelManager::at(int px, int py) {
    for (auto p : panels) if (p->visible && p->contains(px, py)) return p;
    return nullptr;
}

void PanelManager::registerCallback(const std::string& name, std::function<void()> cb) {
    globalCallbacks[name] = cb;
}

void PanelManager::loadUIFromJSON(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "Cannot open: " << filename << std::endl;
        return;
    }
    
    std::string content, line;
    while (std::getline(file, line)) content += line;
    file.close();
    
    auto findValue = [&](const std::string& str, const std::string& key, size_t start) -> std::string {
        std::string search = "\"" + key + "\"";
        size_t pos = str.find(search, start);
        if (pos == std::string::npos) return "";
        size_t colon = str.find(':', pos);
        if (colon == std::string::npos) return "";
        size_t valStart = colon + 1;
        while (valStart < str.length() && (str[valStart] == ' ' || str[valStart] == '\t')) valStart++;
        
        if (str[valStart] == '"') {
            size_t valEnd = str.find('"', valStart + 1);
            return str.substr(valStart + 1, valEnd - valStart - 1);
        } else {
            size_t valEnd = valStart;
            while (valEnd < str.length() && str[valEnd] != ',' && str[valEnd] != '}' && str[valEnd] != ']') valEnd++;
            return str.substr(valStart, valEnd - valStart);
        }
    };
    
    for (auto p : panels) {
        std::string panelSearch = "\"" + p->name + "\"";
        size_t panelPos = content.find(panelSearch);
        if (panelPos == std::string::npos) continue;
        
        size_t buttonsPos = content.find("\"buttons\"", panelPos);
        if (buttonsPos != std::string::npos) {
            size_t arrStart = content.find('[', buttonsPos);
            size_t arrEnd = content.find(']', arrStart);
            std::string buttonsStr = content.substr(arrStart, arrEnd - arrStart);
            
            size_t btnPos = 0;
            while ((btnPos = buttonsStr.find('{', btnPos)) != std::string::npos) {
                size_t btnEnd = buttonsStr.find('}', btnPos);
                std::string btnStr = buttonsStr.substr(btnPos, btnEnd - btnPos);
                
                std::string btnName = findValue(btnStr, "name", 0);
                int x = std::stoi(findValue(btnStr, "x", 0));
                int y = std::stoi(findValue(btnStr, "y", 0));
                int w = std::stoi(findValue(btnStr, "width", 0));
                int h = std::stoi(findValue(btnStr, "height", 0));
                
                std::string colorStr = findValue(btnStr, "color", 0);
                float cr = 0.4f, cg = 0.4f, cb = 0.5f;
                if (colorStr.find('[') != std::string::npos) {
                    sscanf(colorStr.c_str(), "[%f,%f,%f]", &cr, &cg, &cb);
                }
                
                auto it = globalCallbacks.find(btnName);
                if (it != globalCallbacks.end()) {
                    p->addButton(btnName, x, y, w, h, cr, cg, cb, it->second);
                } else {
                    p->addButton(btnName, x, y, w, h, cr, cg, cb, []() {});
                }
                
                btnPos = btnEnd + 1;
            }
        }
        
        size_t labelsPos = content.find("\"labels\"", panelPos);
        if (labelsPos != std::string::npos) {
            size_t arrStart = content.find('[', labelsPos);
            size_t arrEnd = content.find(']', arrStart);
            std::string labelsStr = content.substr(arrStart, arrEnd - arrStart);
            
            size_t lblPos = 0;
            while ((lblPos = labelsStr.find('{', lblPos)) != std::string::npos) {
                size_t lblEnd = labelsStr.find('}', lblPos);
                std::string lblStr = labelsStr.substr(lblPos, lblEnd - lblPos);
                
                std::string text = findValue(lblStr, "text", 0);
                int x = std::stoi(findValue(lblStr, "x", 0));
                int y = std::stoi(findValue(lblStr, "y", 0));
                int fontSize = std::stoi(findValue(lblStr, "fontSize", 0));
                bool bold = findValue(lblStr, "bold", 0) == "true";
                
                std::string colorStr = findValue(lblStr, "color", 0);
                float cr = 0.8f, cg = 0.8f, cb = 0.8f;
                if (colorStr.find('[') != std::string::npos) {
                    sscanf(colorStr.c_str(), "[%f,%f,%f]", &cr, &cg, &cb);
                }
                
                p->addLabel(text, x, y, fontSize, bold, cr, cg, cb);
                lblPos = lblEnd + 1;
            }
        }
    }
}

void PanelManager::loadConfig(const std::string& filename) {
    loadUIFromJSON(filename);
}

void PanelManager::update(int sw, int sh) {
    screenW = sw; screenH = sh;
    int topH = 0, leftW = 220, rightW = 260, consoleH = 150;
    
    for (auto p : panels) {
        if (p->name == "TopBar") topH = p->getH();
        if (p->name == "Console") consoleH = p->getH();
        if (p->name == "Hierarchy") leftW = p->getW();
        if (p->name == "Inspector") rightW = p->getW();
    }
    
    for (auto p : panels) {
        if (p->name == "TopBar") {
            p->setPos(0, 0);
            p->setSize(sw, std::max(30, std::min(400, p->getH())));
        } else if (p->name == "Console") {
            p->setPos(leftW, sh - consoleH);
            p->setSize(sw - leftW - rightW, consoleH);
        } else if (p->name == "Hierarchy") {
            p->setPos(0, topH);
            p->setSize(leftW, sh - topH);
        } else if (p->name == "Inspector") {
            p->setPos(sw - rightW, topH);
            p->setSize(rightW, sh - topH);
        } else if (p->name == "3D Viewport") {
            p->setPos(leftW, topH);
            p->setSize(sw - leftW - rightW, sh - topH - consoleH);
        }
    }
}

void PanelManager::closeMenu() {
    menuOpen = false;
    menuItems.clear();
    menuCallback = nullptr;
}

void PanelManager::saveLayout(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) return;
    file << "{\n";
    for (size_t i = 0; i < panels.size(); i++) {
        Panel* p = panels[i];
        file << "  \"" << p->name << "\": {\n";
        file << "    \"x\": " << p->getX() << ",\n    \"y\": " << p->getY() << ",\n";
        file << "    \"w\": " << p->getW() << ",\n    \"h\": " << p->getH() << ",\n";
        file << "    \"visible\": " << (p->visible ? "true" : "false") << ",\n";
        file << "    \"collapsed\": " << (p->collapsed ? "true" : "false") << "\n";
        file << "  }" << (i < panels.size() - 1 ? "," : "") << "\n";
    }
    file << "}\n";
    file.close();
}

void PanelManager::loadLayout(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return;
    
    std::string content, line;
    while (std::getline(file, line)) content += line;
    file.close();
    
    for (auto p : panels) {
        std::string search = "\"" + p->name + "\"";
        size_t pos = content.find(search);
        if (pos == std::string::npos) continue;
        
        auto parseVal = [&](const std::string& key) -> int {
            size_t kpos = content.find("\"" + key + "\":", pos);
            if (kpos == std::string::npos) return -1;
            size_t start = kpos + key.length() + 4;
            while (start < content.length() && (content[start] == ' ' || content[start] == '\t')) start++;
            return std::stoi(content.substr(start));
        };
        auto parseBool = [&](const std::string& key) -> bool {
            size_t kpos = content.find("\"" + key + "\":", pos);
            if (kpos == std::string::npos) return p->visible;
            size_t truePos = content.find("true", kpos);
            size_t falsePos = content.find("false", kpos);
            return truePos < falsePos && truePos != std::string::npos;
        };
        
        int x = parseVal("x"), y = parseVal("y"), w = parseVal("w"), h = parseVal("h");
        if (x != -1) p->setPos(x, y != -1 ? y : p->getY());
        if (w != -1) p->setSize(w, h != -1 ? h : p->getH());
        p->setVisible(parseBool("visible"));
        p->setCollapsed(parseBool("collapsed"));
    }
}

void PanelManager::onMouseDown(int x, int y) {
    closeMenu();
    
    Panel* panel1 = nullptr;
    Panel* panel2 = nullptr;
    int commonEdge = -1;
    
    for (auto p : panels) {
        if (!p->visible) continue;
        
        int edge = p->getEdge(x, y, 10);
        if (edge != -1 && edge != 4) {
            for (auto other : panels) {
                if (other == p || !other->visible) continue;
                int otherEdge = other->getEdge(x, y, 10);
                
                if ((edge == 3 && otherEdge == 2) || (edge == 2 && otherEdge == 3) ||
                    (edge == 1 && otherEdge == 0) || (edge == 0 && otherEdge == 1)) {
                    panel1 = p;
                    panel2 = other;
                    commonEdge = edge;
                    break;
                }
            }
            if (panel1) break;
        }
    }
    
    if (panel1 && panel2 && panel1->name != "TopBar" && panel2->name != "TopBar") {
        dragging = panel1;
        dragPartner = panel2;
        dragX = x; dragY = y;
        dragW1 = panel1->getW(); dragH1 = panel1->getH();
        dragW2 = panel2->getW(); dragH2 = panel2->getH();
        dragX1 = panel1->getX(); dragY1 = panel1->getY();
        dragX2 = panel2->getX(); dragY2 = panel2->getY();
        dragEdge = commonEdge;
        isDrag = true;
        isResizing = true;
        isDoubleEdge = true;
        SetCapture(GetForegroundWindow());
        return;
    }
    
    for (auto p : panels) {
        if (!p->visible) continue;
        
        int edge = p->getEdge(x, y, 10);
        if (edge != -1 && edge != 4) {
            dragging = p;
            dragPartner = nullptr;
            dragX = x; dragY = y;
            dragW1 = p->getW(); dragH1 = p->getH();
            dragEdge = edge;
            isDrag = true;
            isResizing = true;
            isDoubleEdge = false;
            SetCapture(GetForegroundWindow());
            return;
        }
        
        if (p->onCloseBtn(x, y)) { p->visible = false; return; }
        if (p->onCollapseBtn(x, y)) { p->collapsed = !p->collapsed; return; }
        if (p->onMenuBtn(x, y)) {
            menuOpen = true;
            menuX = x; menuY = y + 15;
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
            dragging = p;
            dragPartner = nullptr;
            dragX = x - p->getX();
            dragY = y - p->getY();
            isDrag = true;
            isResizing = false;
            isDoubleEdge = false;
            SetCapture(GetForegroundWindow());
            return;
        }
    }
    
    if (menuOpen) {
        int idx = (y - menuY) / 20;
        if (idx >= 0 && idx < (int)menuItems.size() && menuCallback) menuCallback(idx);
        closeMenu();
    }
}

void PanelManager::onMouseMove(int x, int y) {
    if (!isDrag || !dragging) return;
    
    if (isResizing) {
        int dx = x - dragX, dy = y - dragY;
        if (dx > 500) dx = 500; if (dx < -500) dx = -500;
        if (dy > 500) dy = 500; if (dy < -500) dy = -500;
        
        if (isDoubleEdge && dragPartner) {
            if (dragEdge == 3) {
                int newH1 = dragH1 + dy;
                int newH2 = dragH2 - dy;
                int newY2 = dragY2 + dy;
                if (newH1 >= 100 && newH2 >= 100) {
                    dragging->setSize(dragging->getW(), newH1);
                    dragPartner->setPos(dragPartner->getX(), newY2);
                    dragPartner->setSize(dragPartner->getW(), newH2);
                }
            }
            else if (dragEdge == 2) {
                int newH1 = dragH1 - dy;
                int newH2 = dragH2 + dy;
                int newY1 = dragY1 + dy;
                if (newH1 >= 100 && newH2 >= 100) {
                    dragging->setPos(dragging->getX(), newY1);
                    dragging->setSize(dragging->getW(), newH1);
                    dragPartner->setSize(dragPartner->getW(), newH2);
                }
            }
            else if (dragEdge == 1) {
                int newW1 = dragW1 + dx;
                int newW2 = dragW2 - dx;
                int newX2 = dragX2 + dx;
                if (newW1 >= 100 && newW2 >= 100) {
                    dragging->setSize(newW1, dragging->getH());
                    dragPartner->setPos(newX2, dragPartner->getY());
                    dragPartner->setSize(newW2, dragPartner->getH());
                }
            }
            else if (dragEdge == 0) {
                int newW1 = dragW1 - dx;
                int newW2 = dragW2 + dx;
                int newX1 = dragX1 + dx;
                if (newW1 >= 100 && newW2 >= 100) {
                    dragging->setPos(newX1, dragging->getY());
                    dragging->setSize(newW1, dragging->getH());
                    dragPartner->setSize(newW2, dragPartner->getH());
                }
            }
        } else {
            int nx = dragging->getX(), ny = dragging->getY();
            int nw = dragW1, nh = dragH1;
            
            if (dragEdge == 0) { nw = dragW1 - dx; nx = dragging->getX() + dx; }
            else if (dragEdge == 1) { nw = dragW1 + dx; }
            else if (dragEdge == 2) { nh = dragH1 - dy; ny = dragging->getY() + dy; }
            else if (dragEdge == 3) { nh = dragH1 + dy; }
            
            if (nw < 100) nw = 100;
            if (nh < 30) nh = 30;
            
            int delta = 0;
            if (dragEdge == 0) { delta = nw - dragW1; nx = dragging->getX() + (dragW1 - nw); }
            else if (dragEdge == 1) { delta = nw - dragW1; }
            else if (dragEdge == 2) { delta = nh - dragH1; ny = dragging->getY() + (dragH1 - nh); }
            else if (dragEdge == 3) { delta = nh - dragH1; }
            
            dragging->setPos(nx, ny);
            dragging->setSize(nw, nh);
            
            if (dragging->name == "TopBar" && dragEdge == 3) {
                for (auto other : panels) {
                    if (other == dragging || !other->visible) continue;
                    int newY = other->getY() + delta;
                    if (newY >= dragging->getH()) other->setPos(other->getX(), newY);
                }
            }
            else if (dragging->name == "Console" && dragEdge == 3) {
                int newH = dragging->getH();
                if (newH >= 100 && newH <= 400) {
                    dragging->setPos(dragging->getX(), screenH - newH);
                }
            }
            else {
                for (auto other : panels) {
                    if (other == dragging || !other->visible) continue;
                    if (dragEdge == 1 && abs(other->r.x - dragging->r.right) < 5) {
                        int newW = other->getW() - dx, newX = other->getX() + dx;
                        if (newW >= 100) { other->setPos(newX, other->getY()); other->setSize(newW, other->getH()); }
                    }
                    else if (dragEdge == 0 && abs(other->r.right - dragging->r.x) < 5) {
                        int newW = other->getW() + dx;
                        if (newW >= 100) other->setSize(newW, other->getH());
                    }
                    else if (dragEdge == 3 && abs(other->r.y - dragging->r.bottom) < 5) {
                        int newH = other->getH() - dy, newY = other->getY() + dy;
                        if (newH >= 50) { other->setPos(other->getX(), newY); other->setSize(other->getW(), newH); }
                    }
                    else if (dragEdge == 2 && abs(other->r.bottom - dragging->r.y) < 5) {
                        int newH = other->getH() + dy;
                        if (newH >= 50) other->setSize(other->getW(), newH);
                    }
                }
            }
        }
        
        dragX = x; dragY = y;
        dragW1 = dragging->getW(); dragH1 = dragging->getH();
        if (dragPartner) {
            dragW2 = dragPartner->getW(); dragH2 = dragPartner->getH();
            dragX2 = dragPartner->getX(); dragY2 = dragPartner->getY();
        }
    } else {
        dragging->setPos(x - dragX, y - dragY);
    }
}

void PanelManager::onMouseUp(int x, int y) {
    isDrag = false;
    dragging = nullptr;
    dragPartner = nullptr;
    isResizing = false;
    isDoubleEdge = false;
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
        int w = 100, h = menuItems.size() * 20;
        glBegin(GL_QUADS);
        glColor3f(0.15f, 0.15f, 0.18f);
        glVertex2f(menuX, menuY); glVertex2f(menuX + w, menuY);
        glVertex2f(menuX + w, menuY + h); glVertex2f(menuX, menuY + h);
        glColor3f(0.25f, 0.25f, 0.3f);
        for (size_t i = 0; i < menuItems.size(); i++) {
            int yy = menuY + i * 20;
            glVertex2f(menuX, yy); glVertex2f(menuX + w, yy);
            glVertex2f(menuX + w, yy + 20); glVertex2f(menuX, yy + 20);
        }
        glEnd();
        for (size_t i = 0; i < menuItems.size(); i++) {
            render.drawText(menuX + 5, menuY + i * 20 + 5, menuItems[i], 0.9f, 0.9f, 0.9f);
        }
    }
}