#ifndef OBJECTUI_H
#define OBJECTUI_H

#include <string>
#include <vector>
#include <map>
#include <functional>
#include "../Core/Render/Win32/RenderUI.h"
#include "Panels.h"

class UIObject {
protected:
    std::string name;
    int x, y;        // Исходные координаты относительно панели
    int w, h;        // Ширина и высота
    int ax, ay;      // Актуальные координаты на экране
    PanelType attachedPanel;

public:
    UIObject(const std::string& n, int ix, int iy, int iw, int ih);
    virtual ~UIObject() = default;
    
    virtual void render(RenderUI& r) const = 0;
    virtual void onClick(int x, int y) = 0;
    
    bool containsPoint(int px, int py) const;
    void updatePosition(int panelX, int panelY);
    
    // Геттеры
    const std::string& getName() const { return name; }
    int getX() const { return x; }
    int getY() const { return y; }
    int getW() const { return w; }
    int getH() const { return h; }
    int getAX() const { return ax; }
    int getAY() const { return ay; }
    PanelType getAttachedPanel() const { return attachedPanel; }
    
    // Сеттеры
    void setPanel(PanelType p) { attachedPanel = p; }
};

class Button : public UIObject {
private:
    std::function<void()> callback;

public:
    Button(const std::string& n, int ix, int iy, int iw, int ih, std::function<void()> cb);
    void render(RenderUI& r) const override;
    void onClick(int x, int y) override;
};

class ObjectUI {
private:
    std::vector<UIObject*> objects;
    std::map<std::string, UIObject*> objectMap;

public:
    ObjectUI();
    ~ObjectUI();
    std::vector<UIObject*> getObjects() { return objects; };
    void createButton(const std::string& n, int x, int y, int w, int h, std::function<void()> cb);
    void attachToPanel(const std::string& n, PanelType p);
    void updatePositions(int sw, int sh, const Panels& panels);
    void render(RenderUI& r, int w, int h, const Panels& panels);
    void handleClick(int x, int y, int w, int h, const Panels& panels);
    
    // Новые методы для расчета минимальных размеров панелей
    int getMinWidthForPanel(PanelType panel) const;
    int getMinHeightForPanel(PanelType panel) const;
    
    // Получить все объекты на панели
    std::vector<UIObject*> getObjectsOnPanel(PanelType panel) const;
};

#endif