#include "ObjectUI.h"
#include <iostream>

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
}

void ObjectUI::handleClick(int x, int y, int w, int h, const Panels& panels) {
    updatePositions(w, h, panels);
    
    for (auto obj : objects) {
        if (obj->containsPoint(x, y)) {
            obj->onClick(x, y);
            return;
        }
    }
}