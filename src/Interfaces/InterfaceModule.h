#ifndef INTERFACE_MODULE_H
#define INTERFACE_MODULE_H

#include <string>
#include <functional>
#include <vector>
#include <memory>
#include <iostream>  
// Типы модулей
enum class ModuleType {
    RENDER_VIEW,        // Окно рендера
    HIERARCHY,          // Иерархия сцены
    ASSETS,             // Окно ассетов
    OBJECT_PROPERTIES,  // Свойства объекта
    SCENE_VIEW,         // Редактор сцены
    CONSOLE,            // Консоль
    STATISTICS,         // Статистика
    CUSTOM              // Пользовательский модуль
};

// Структура для позиции и размера модуля
struct ModuleRect {
    float x, y;           // Позиция (0-1 относительно окна)
    float width, height;  // Размер (0-1 относительно окна)
    
    ModuleRect(float x = 0, float y = 0, float w = 1, float h = 1) 
        : x(x), y(y), width(w), height(h) {}
};

// Предварительное объявление для дружественных классов
class ModuleManager;

// Базовый класс модуля интерфейса
class InterfaceModule {
public:
    InterfaceModule(ModuleType type, const std::string& name, const ModuleRect& rect);
    virtual ~InterfaceModule() = default;
    
    // Виртуальные методы для реализации в дочерних классах
    virtual void Draw() {
        // Базовая реализация
        std::cout << "Module: " << name 
                  << " at (" << rect.x << ", " << rect.y 
                  << ") size (" << rect.width << "x" << rect.height << ")" << std::endl;
    }
    
    virtual void Update() {}                          // Обновление состояния
    virtual void OnResize(float newWidth, float newHeight) {} // При изменении размера
    
    // Базовые методы управления
    void DrawHeader();                                // Отрисовка заголовка с кнопками
    bool IsHovered() const;                           // Проверка наведения мыши
    bool IsResizing() const;                          // Проверка изменения размера
    
    // Геттеры и сеттеры
    ModuleType GetType() const { return type; }
    std::string GetName() const { return name; }
    ModuleRect GetRect() const { return rect; }
    void SetRect(const ModuleRect& newRect) { rect = newRect; }
    bool IsVisible() const { return visible; }
    void SetVisible(bool visible) { this->visible = visible; }
    
    // Обработка input
    void ProcessMouseClick(float mouseX, float mouseY);
    void ProcessMouseDrag(float deltaX, float deltaY);
    
protected:
    ModuleType type;
    std::string name;
    ModuleRect rect;
    bool visible;
    bool isResizing;
    int resizeEdge; // 0=left, 1=right, 2=top, 3=bottom, -1=none
};

// Менеджер модулей
class ModuleManager {
public:
    static ModuleManager& GetInstance();
    
    // Управление модулями
    void AddModule(std::shared_ptr<InterfaceModule> module);
    void RemoveModule(const std::string& name);
    std::shared_ptr<InterfaceModule> GetModule(const std::string& name);
    std::vector<std::shared_ptr<InterfaceModule>>& GetModules() { return modules; }
    
    // Поиск модулей
    std::shared_ptr<InterfaceModule> GetModuleAt(float x, float y);
    std::vector<std::shared_ptr<InterfaceModule>> GetModulesByType(ModuleType type);
    
    // Обновление и отрисовка
    void UpdateModules();
    void DrawModules();
    
    // Обработка input
    void ProcessMouseClick(float x, float y);
    void ProcessMouseDrag(float deltaX, float deltaY);
    void ProcessMouseRelease();
    
    // Сохранение/загрузка layout
    void SaveLayout(const std::string& filename);
    void LoadLayout(const std::string& filename);
    
private:
    ModuleManager() = default;
    std::vector<std::shared_ptr<InterfaceModule>> modules;
    std::shared_ptr<InterfaceModule> activeModule;
    std::shared_ptr<InterfaceModule> resizingModule;
};

#endif