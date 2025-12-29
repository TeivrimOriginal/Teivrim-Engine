#include "InterfaceModule.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>

// Базовый модуль
InterfaceModule::InterfaceModule(ModuleType type, const std::string& name, const ModuleRect& rect)
    : type(type), name(name), rect(rect), visible(true), isResizing(false), resizeEdge(-1) {}

void InterfaceModule::DrawHeader() {
    std::cout << "[" << name << "] ";
    std::cout << "Type: " << static_cast<int>(type) << " ";
    std::cout << "Visible: " << (visible ? "Yes" : "No") << std::endl;
}

bool InterfaceModule::IsHovered() const {
    // Заглушка - в реальности проверяем позицию мыши
    return false;
}

bool InterfaceModule::IsResizing() const {
    return isResizing;
}

void InterfaceModule::ProcessMouseClick(float mouseX, float mouseY) {
    // Проверяем, кликнули ли на границу для изменения размера
    const float resizeBorder = 0.02f; // 2% от размера модуля
    
    float left = rect.x;
    float right = rect.x + rect.width;
    float top = rect.y;
    float bottom = rect.y + rect.height;
    
    isResizing = false;
    resizeEdge = -1;
    
    if (std::abs(mouseX - left) < resizeBorder) {
        isResizing = true;
        resizeEdge = 0; // left
    } else if (std::abs(mouseX - right) < resizeBorder) {
        isResizing = true;
        resizeEdge = 1; // right
    } else if (std::abs(mouseY - top) < resizeBorder) {
        isResizing = true;
        resizeEdge = 2; // top
    } else if (std::abs(mouseY - bottom) < resizeBorder) {
        isResizing = true;
        resizeEdge = 3; // bottom
    }
}

void InterfaceModule::ProcessMouseDrag(float deltaX, float deltaY) {
    if (!isResizing) return;
    
    switch (resizeEdge) {
        case 0: // left
            rect.x += deltaX;
            rect.width -= deltaX;
            break;
        case 1: // right
            rect.width += deltaX;
            break;
        case 2: // top
            rect.y += deltaY;
            rect.height -= deltaY;
            break;
        case 3: // bottom
            rect.height += deltaY;
            break;
    }
    
    // Ограничиваем размеры
    rect.width = std::max(0.1f, std::min(1.0f, rect.width));
    rect.height = std::max(0.1f, std::min(1.0f, rect.height));
    rect.x = std::max(0.0f, std::min(1.0f - rect.width, rect.x));
    rect.y = std::max(0.0f, std::min(1.0f - rect.height, rect.y));
}

// Менеджер модулей
ModuleManager& ModuleManager::GetInstance() {
    static ModuleManager instance;
    return instance;
}

void ModuleManager::AddModule(std::shared_ptr<InterfaceModule> module) {
    modules.push_back(module);
    std::cout << "Module added: " << module->GetName() << std::endl;
}

void ModuleManager::RemoveModule(const std::string& name) {
    auto it = std::remove_if(modules.begin(), modules.end(),
        [&](const std::shared_ptr<InterfaceModule>& module) {
            return module->GetName() == name;
        });
    
    if (it != modules.end()) {
        modules.erase(it, modules.end());
        std::cout << "Module removed: " << name << std::endl;
    } else {
        std::cout << "Module not found: " << name << std::endl;
    }
}

std::shared_ptr<InterfaceModule> ModuleManager::GetModule(const std::string& name) {
    auto it = std::find_if(modules.begin(), modules.end(),
        [&](const std::shared_ptr<InterfaceModule>& module) {
            return module->GetName() == name;
        });
    return (it != modules.end()) ? *it : nullptr;
}

std::shared_ptr<InterfaceModule> ModuleManager::GetModuleAt(float x, float y) {
    // Проверяем с конца, чтобы верхние модули получали клики первыми
    for (auto it = modules.rbegin(); it != modules.rend(); ++it) {
        ModuleRect rect = (*it)->GetRect();
        if (x >= rect.x && x <= rect.x + rect.width &&
            y >= rect.y && y <= rect.y + rect.height) {
            return *it;
        }
    }
    return nullptr;
}

std::vector<std::shared_ptr<InterfaceModule>> ModuleManager::GetModulesByType(ModuleType type) {
    std::vector<std::shared_ptr<InterfaceModule>> result;
    for (auto& module : modules) {
        if (module->GetType() == type) {
            result.push_back(module);
        }
    }
    return result;
}

void ModuleManager::UpdateModules() {
    for (auto& module : modules) {
        if (module->IsVisible()) {
            module->Update();
        }
    }
}

void ModuleManager::DrawModules() {
    // Сортируем модули по z-order (сначала отрисовываем нижние)
    std::sort(modules.begin(), modules.end(),
        [](const std::shared_ptr<InterfaceModule>& a, 
           const std::shared_ptr<InterfaceModule>& b) {
            // Простая сортировка по Y, затем по X
            if (a->GetRect().y != b->GetRect().y)
                return a->GetRect().y < b->GetRect().y;
            return a->GetRect().x < b->GetRect().x;
        });
    
    for (auto& module : modules) {
        if (module->IsVisible()) {
            module->DrawHeader();
            module->Draw();
            std::cout << "---" << std::endl;
        }
    }
}

void ModuleManager::ProcessMouseClick(float x, float y) {
    activeModule = GetModuleAt(x, y);
    if (activeModule) {
        std::cout << "Clicked on module: " << activeModule->GetName() << std::endl;
        activeModule->ProcessMouseClick(x, y);
        if (activeModule->IsResizing()) {
            resizingModule = activeModule;
            std::cout << "Resizing module: " << activeModule->GetName() << std::endl;
        }
    } else {
        std::cout << "Clicked on empty space at (" << x << ", " << y << ")" << std::endl;
    }
}

void ModuleManager::ProcessMouseDrag(float deltaX, float deltaY) {
    if (resizingModule) {
        resizingModule->ProcessMouseDrag(deltaX, deltaY);
    } else if (activeModule) {
        // Перемещение модуля
        ModuleRect rect = activeModule->GetRect();
        rect.x += deltaX;
        rect.y += deltaY;
        
        // Ограничиваем позицию
        rect.x = std::max(0.0f, std::min(1.0f - rect.width, rect.x));
        rect.y = std::max(0.0f, std::min(1.0f - rect.height, rect.y));
        
        activeModule->SetRect(rect);
        std::cout << "Moving module: " << activeModule->GetName() 
                  << " to (" << rect.x << ", " << rect.y << ")" << std::endl;
    }
}

void ModuleManager::ProcessMouseRelease() {
    if (resizingModule) {
        std::cout << "Finished resizing module: " << resizingModule->GetName() << std::endl;
        resizingModule = nullptr;
    }
    activeModule = nullptr;
}

void ModuleManager::SaveLayout(const std::string& filename) {
    std::ofstream file(filename);
    if (file.is_open()) {
        for (auto& module : modules) {
            ModuleRect rect = module->GetRect();
            file << static_cast<int>(module->GetType()) << " "
                 << module->GetName() << " "
                 << rect.x << " " << rect.y << " "
                 << rect.width << " " << rect.height << "\n";
        }
        file.close();
        std::cout << "Layout saved to: " << filename << std::endl;
    } else {
        std::cout << "Failed to save layout to: " << filename << std::endl;
    }
}

void ModuleManager::LoadLayout(const std::string& filename) {
    std::ifstream file(filename);
    if (file.is_open()) {
        modules.clear();
        std::string line;
        
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            int type;
            std::string name;
            float x, y, w, h;
            
            if (iss >> type >> name >> x >> y >> w >> h) {
                ModuleRect rect(x, y, w, h);
                // Создаем базовый модуль с указанными параметрами
                auto module = std::make_shared<InterfaceModule>(
                    static_cast<ModuleType>(type), name, rect);
                modules.push_back(module);
                std::cout << "Loaded module: " << name << " type: " << type << std::endl;
            }
        }
        file.close();
        std::cout << "Layout loaded from: " << filename << std::endl;
    } else {
        std::cout << "Failed to load layout from: " << filename << std::endl;
    }
}