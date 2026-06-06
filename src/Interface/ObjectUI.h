// ObjectUI.h
#ifndef OBJECTUI_H
#define OBJECTUI_H

#include <string>
#include <vector>
#include "../Core/Render/RenderUI.h"
#include "Panels.h"

class ObjectUI {
public:
    ObjectUI();
    ~ObjectUI();
    
    void render(RenderUI& r, int w, int h, PanelManager& panels);
    void handleClick(int x, int y, int w, int h, PanelManager& panels);
    void handleKeyboardInput(WPARAM wParam, char c);
    void setVulkan(class Vulkan* vk) { m_vulkan = vk; }
    
    bool isAnyInputActive() const { return m_isEditing; }
    
private:
    void drawInputField(RenderUI& r, const std::string& value, int x, int y, int w, int h, bool isActive);
    void applyFieldValue();
    void updateFieldValuesFromObject();
    
    std::vector<std::string> m_fieldValues;
    std::vector<std::string> m_fieldOriginalValues;
    int m_activeFieldIndex;
    bool m_isEditing;
    class Vulkan* m_vulkan;
};

#endif