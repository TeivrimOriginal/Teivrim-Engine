#include "MainInterface.h"
#include <iostream>

// RenderViewModule
RenderViewModule::RenderViewModule(const ModuleRect& rect)
    : InterfaceModule(ModuleType::RENDER_VIEW, "Render View", rect) {}

void RenderViewModule::Draw() {
    std::cout << "=== RENDER VIEW ===" << std::endl;
    std::cout << "Displaying 3D scene preview" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  - WASD: Move camera" << std::endl;
    std::cout << "  - Mouse: Look around" << std::endl;
    std::cout << "  - Scroll: Zoom" << std::endl;
}

void RenderViewModule::Update() {
    // Обновление состояния рендер вью
}

// HierarchyModule
HierarchyModule::HierarchyModule(const ModuleRect& rect)
    : InterfaceModule(ModuleType::HIERARCHY, "Hierarchy", rect) {
    sceneObjects = {"Main Camera", "Cube_01", "Light_Directional", "Player", "Floor", "Sphere_01"};
}

void HierarchyModule::Draw() {
    std::cout << "=== SCENE HIERARCHY ===" << std::endl;
    for (size_t i = 0; i < sceneObjects.size(); i++) {
        std::cout << "  [" << i << "] " << sceneObjects[i] << std::endl;
    }
    std::cout << "Total objects: " << sceneObjects.size() << std::endl;
}

// AssetsModule
AssetsModule::AssetsModule(const ModuleRect& rect)
    : InterfaceModule(ModuleType::ASSETS, "Assets", rect) {
    assets = {"model.obj", "texture.png", "material.mat", "scene.fbx", 
              "sound.wav", "script.lua", "shader.glsl", "config.ini"};
}

void AssetsModule::Draw() {
    std::cout << "=== ASSETS BROWSER ===" << std::endl;
    std::cout << "Project Assets:" << std::endl;
    for (const auto& asset : assets) {
        std::cout << "  - " << asset << std::endl;
    }
    std::cout << "Total assets: " << assets.size() << std::endl;
}

// PropertiesModule
PropertiesModule::PropertiesModule(const ModuleRect& rect)
    : InterfaceModule(ModuleType::OBJECT_PROPERTIES, "Properties", rect) {
    selectedObject = "Cube_01";
}

void PropertiesModule::Draw() {
    std::cout << "=== OBJECT PROPERTIES ===" << std::endl;
    std::cout << "Selected: " << selectedObject << std::endl;
    std::cout << "Transform:" << std::endl;
    std::cout << "  Position: (0.0, 1.5, -2.0)" << std::endl;
    std::cout << "  Rotation: (0.0, 45.0, 0.0)" << std::endl;
    std::cout << "  Scale:    (1.0, 1.0, 1.0)" << std::endl;
    std::cout << "Components:" << std::endl;
    std::cout << "  - MeshRenderer" << std::endl;
    std::cout << "  - BoxCollider" << std::endl;
    std::cout << "  - Rigidbody" << std::endl;
}

// MainInterface
MainInterface::MainInterface(GLFWwindow* window)
    : window(window), moduleManager(ModuleManager::GetInstance()),
      lastMouseX(0), lastMouseY(0), mousePressed(false) {
    std::cout << "MainInterface created" << std::endl;
}

MainInterface::~MainInterface() {
    std::cout << "MainInterface destroyed" << std::endl;
}

void MainInterface::Initialize() {
    // Создаем стандартный layout
    CreateDefaultLayout();
    
    std::cout << "\n=== MAIN INTERFACE INITIALIZED ===" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  - Click & drag module edges: Resize" << std::endl;
    std::cout << "  - Click & drag module center: Move" << std::endl;
    std::cout << "  - F1: Reset to default layout" << std::endl;
    std::cout << "  - F5: Save layout" << std::endl;
    std::cout << "  - F9: Load layout" << std::endl;
    std::cout << "=================================\n" << std::endl;
}

void MainInterface::CreateDefaultLayout() {
    // Очищаем существующие модули
    auto& modules = moduleManager.GetModules();
    modules.clear();
    
    // Создаем стандартные модули
    auto hierarchy = std::make_shared<HierarchyModule>();
    auto renderView = std::make_shared<RenderViewModule>();
    auto assets = std::make_shared<AssetsModule>();
    auto properties = std::make_shared<PropertiesModule>();
    
    moduleManager.AddModule(hierarchy);
    moduleManager.AddModule(renderView);
    moduleManager.AddModule(assets);
    moduleManager.AddModule(properties);
    
    std::cout << "Default layout created with 4 modules" << std::endl;
}

void MainInterface::Update() {
    moduleManager.UpdateModules();
}

void MainInterface::Render() {
    // В реальности здесь будет отрисовка GUI
    // Для консольной версии просто выводим информацию
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "FRAME RENDER" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    
    moduleManager.DrawModules();
    
    std::cout << std::string(50, '=') << std::endl;
}

void MainInterface::ProcessInput() {
    // Обработка специальных клавиш интерфейса
    static bool f1Pressed = false, f5Pressed = false, f9Pressed = false;
    
    if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS && !f1Pressed) {
        CreateDefaultLayout();
        f1Pressed = true;
    } else if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_RELEASE) {
        f1Pressed = false;
    }
    
    if (glfwGetKey(window, GLFW_KEY_F5) == GLFW_PRESS && !f5Pressed) {
        moduleManager.SaveLayout("layout.txt");
        f5Pressed = true;
    } else if (glfwGetKey(window, GLFW_KEY_F5) == GLFW_RELEASE) {
        f5Pressed = false;
    }
    
    if (glfwGetKey(window, GLFW_KEY_F9) == GLFW_PRESS && !f9Pressed) {
        moduleManager.LoadLayout("layout.txt");
        f9Pressed = true;
    } else if (glfwGetKey(window, GLFW_KEY_F9) == GLFW_RELEASE) {
        f9Pressed = false;
    }
}