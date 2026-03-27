#include "ObjectUI.h"
#include <iostream>
#include <algorithm>

UIObject::UIObject(const std::string& n, int ix, int iy, int iw, int ih)
    : name(n), x(ix), y(iy), w(iw), h(ih), ax(ix), ay(iy), attachedPanel(PanelType::None) {}

bool UIObject::containsPoint(int px, int py) const {
    return px >= ax && px <= ax + w && py >= ay && py <= ay + h;
}

void UIObject::updatePosition(int panelX, int panelY) {
    ax = panelX + x;
    ay = panelY + y;
}

Button::Button(const std::string& n, int ix, int iy, int iw, int ih, std::function<void()> cb)
    : UIObject(n, ix, iy, iw, ih), callback(cb) {}

void Button::render(RenderUI& r) const {
    glColor3f(0.5f, 0.5f, 0.6f);
    glVertex2f(ax, ay);
    glVertex2f(ax + w, ay);
    glVertex2f(ax + w, ay + h);
    glVertex2f(ax, ay + h);
    
    glColor3f(0.3f, 0.3f, 0.3f);
    glVertex2f(ax, ay);
    glVertex2f(ax + w, ay);
    glVertex2f(ax + w, ay + 2);
    glVertex2f(ax, ay + 2);
    
    glVertex2f(ax, ay + h - 2);
    glVertex2f(ax + w, ay + h - 2);
    glVertex2f(ax + w, ay + h);
    glVertex2f(ax, ay + h);
    
    glVertex2f(ax, ay);
    glVertex2f(ax + 2, ay);
    glVertex2f(ax + 2, ay + h);
    glVertex2f(ax, ay + h);
    
    glVertex2f(ax + w - 2, ay);
    glVertex2f(ax + w, ay);
    glVertex2f(ax + w, ay + h);
    glVertex2f(ax + w - 2, ay + h);
}

void Button::onClick(int x, int y) {
    if (callback) {
        std::cout << "Button '" << name << "' clicked!" << std::endl;
        callback();
    }
}

DropdownMenu::DropdownMenu() {}

void DropdownMenu::show(int px, int py, const std::vector<std::string>& menuItems, std::function<void(int)> cb) {
    x = px;
    y = py;
    items = menuItems;
    callback = cb;
    visible = true;
}

void DropdownMenu::hide() {
    visible = false;
    items.clear();
    callback = nullptr;
}

void DropdownMenu::render(RenderUI& renderer) const {
    if (!visible) return;
    
    int itemHeight = 20;
    int width = 120;
    int height = items.size() * itemHeight;
    
    glColor3f(0.15f, 0.15f, 0.15f);
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width, y + height);
    glVertex2f(x, y + height);
    
    glColor3f(0.3f, 0.3f, 0.3f);
    for (size_t i = 0; i < items.size(); i++) {
        int itemY = y + i * itemHeight;
        glVertex2f(x, itemY);
        glVertex2f(x + width, itemY);
        glVertex2f(x + width, itemY + itemHeight);
        glVertex2f(x, itemY + itemHeight);
        
        renderer.drawText(x + 5, itemY + 5, items[i], 1.0f, 1.0f, 1.0f);
    }
}

bool DropdownMenu::handleClick(int clickX, int clickY) {
    if (!visible) return false;
    
    int itemHeight = 20;
    int width = 120;
    int height = items.size() * itemHeight;
    
    if (clickX >= x && clickX <= x + width && clickY >= y && clickY <= y + height) {
        int index = (clickY - y) / itemHeight;
        if (index >= 0 && index < (int)items.size() && callback) {
            callback(index);
        }
        hide();
        return true;
    }
    
    hide();
    return false;
}

ObjectUI::ObjectUI() {}

ObjectUI::~ObjectUI() {
    for (auto obj : objects) {
        delete obj;
    }
}

void ObjectUI::createButton(const std::string& n, int x, int y, int w, int h, std::function<void()> cb) {
    Button* btn = new Button(n, x, y, w, h, cb);
    objects.push_back(btn);
    objectMap[n] = btn;
}

void ObjectUI::attachToPanel(const std::string& n, PanelType p) {
    auto it = objectMap.find(n);
    if (it != objectMap.end()) {
        it->second->setPanel(p);
    }
}

void ObjectUI::updatePositions(int sw, int sh, const Panels& panels) {
    for (auto obj : objects) {
        if (obj->getAttachedPanel() != PanelType::None) {
            int x1, y1, x2, y2;
            panels.getPanelBounds(obj->getAttachedPanel(), sw, sh, x1, y1, x2, y2);
            obj->updatePosition(x1, y1);
        }
    }
}

void ObjectUI::render(RenderUI& r, int w, int h, const Panels& panels) {
    updatePositions(w, h, panels);
    
    for (auto obj : objects) {
        glBegin(GL_QUADS);
        obj->render(r);
        glEnd();
    }
    
    for (auto obj : objects) {
        int textX = obj->getAX() + (obj->getW() - (int)obj->getName().length() * 8) / 2;
        int textY = obj->getAY() + (obj->getH() - 12) / 2;
        r.drawText(textX, textY, obj->getName(), 1.0f, 1.0f, 1.0f);
    }
    
    activeMenu.render(r);
}

void ObjectUI::handleClick(int x, int y, int w, int h, const Panels& panels) {
    if (activeMenu.isVisible()) {
        if (activeMenu.handleClick(x, y)) {
            return;
        }
    }
    
    updatePositions(w, h, panels);
    
    for (auto obj : objects) {
        if (obj->containsPoint(x, y)) {
            obj->onClick(x, y);
            return;
        }
    }
}

void ObjectUI::showDropdown(int x, int y, const std::vector<std::string>& items, std::function<void(int)> callback) {
    activeMenu.show(x, y, items, callback);
}

std::vector<UIObject*> ObjectUI::getObjectsOnPanel(PanelType panel) const {
    std::vector<UIObject*> result;
    for (auto obj : objects) {
        if (obj->getAttachedPanel() == panel) {
            result.push_back(obj);
        }
    }
    return result;
}

int ObjectUI::getMinWidthForPanel(PanelType panel) const {
    auto panelObjects = getObjectsOnPanel(panel);
    if (panelObjects.empty()) {
        switch (panel) {
            case PanelType::Left:
            case PanelType::Right:
                return 50;
            case PanelType::Top:
            case PanelType::Bottom:
                return 100;
            case PanelType::Floating:
                return 150;
            default:
                return 50;
        }
    }
    
    int maxWidth = 0;
    int maxXPlusWidth = 0;
    
    for (auto obj : panelObjects) {
        if (obj->getW() > maxWidth) {
            maxWidth = obj->getW();
        }
        int rightEdge = obj->getX() + obj->getW();
        if (rightEdge > maxXPlusWidth) {
            maxXPlusWidth = rightEdge;
        }
    }
    
    int requiredWidth = maxXPlusWidth + 20;
    
    if (panel == PanelType::Top || panel == PanelType::Bottom) {
        requiredWidth = std::max(requiredWidth, maxWidth + 20);
    }
    
    return requiredWidth;
}

int ObjectUI::getMinHeightForPanel(PanelType panel) const {
    auto panelObjects = getObjectsOnPanel(panel);
    if (panelObjects.empty()) {
        switch (panel) {
            case PanelType::Left:
            case PanelType::Right:
                return 100;
            case PanelType::Top:
            case PanelType::Bottom:
                return 30;
            case PanelType::Floating:
                return 100;
            default:
                return 50;
        }
    }
    
    int maxHeight = 0;
    int maxYPlusHeight = 0;
    
    for (auto obj : panelObjects) {
        if (obj->getH() > maxHeight) {
            maxHeight = obj->getH();
        }
        int bottomEdge = obj->getY() + obj->getH();
        if (bottomEdge > maxYPlusHeight) {
            maxYPlusHeight = bottomEdge;
        }
    }
    
    int requiredHeight = maxYPlusHeight + 20;
    
    if (panel == PanelType::Left || panel == PanelType::Right) {
        requiredHeight = std::max(requiredHeight, maxHeight + 20);
    }
    
    return requiredHeight;
}