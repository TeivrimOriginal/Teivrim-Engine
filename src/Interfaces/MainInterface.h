#ifndef MAIN_INTERFACE_H
#define MAIN_INTERFACE_H

#include "InterfaceModule.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// Конкретные реализации модулей
class RenderViewModule : public InterfaceModule {
public:
    RenderViewModule(const ModuleRect& rect = ModuleRect(0.2f, 0.0f, 0.6f, 0.7f));
    void Draw() override;
    void Update() override;
};

class HierarchyModule : public InterfaceModule {
public:
    HierarchyModule(const ModuleRect& rect = ModuleRect(0.0f, 0.0f, 0.2f, 1.0f));
    void Draw() override;
    
private:
    std::vector<std::string> sceneObjects;
};

class AssetsModule : public InterfaceModule {
public:
    AssetsModule(const ModuleRect& rect = ModuleRect(0.0f, 0.7f, 0.2f, 0.3f));
    void Draw() override;
    
private:
    std::vector<std::string> assets;
};

class PropertiesModule : public InterfaceModule {
public:
    PropertiesModule(const ModuleRect& rect = ModuleRect(0.8f, 0.0f, 0.2f, 1.0f));
    void Draw() override;
    
private:
    std::string selectedObject;
};

// Главный интерфейс
class MainInterface {
public:
    MainInterface(GLFWwindow* window);
    ~MainInterface();
    
    void Initialize();
    void Update();
    void Render();
    void ProcessInput();
    
    // Создание стандартного layout
    void CreateDefaultLayout();
    
private:
    GLFWwindow* window;
    ModuleManager& moduleManager;
    
    // Состояние мыши
    double lastMouseX, lastMouseY;
    bool mousePressed;
};

#endif