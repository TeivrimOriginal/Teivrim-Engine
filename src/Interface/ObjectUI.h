#ifndef OBJECTUI_H
#define OBJECTUI_H

#include <string>
#include <vector>
#include <functional>
#include "Panels.h"

class ObjectUI {
public:
    ObjectUI() {}
    ~ObjectUI() {}
    
    void render(RenderUI& r, int w, int h, PanelManager& panels) {
        // Пустой, все UI теперь в панелях
    }
    
    void handleClick(int x, int y, int w, int h, PanelManager& panels) {
        // Пустой
    }
};

#endif