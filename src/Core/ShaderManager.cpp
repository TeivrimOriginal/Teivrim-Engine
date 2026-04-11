#include "ShaderManager.h"
#include "Core/Render/Win32/GraphicsApiSupport/VkInit.h"
#include <iostream>

ShaderManager& ShaderManager::getInstance() {
    static ShaderManager instance;
    return instance;
}

bool ShaderManager::createShaders() {
    std::cout << "[ShaderManager] Shaders are handled by VkInit internally" << std::endl;
    return true;
}

bool ShaderManager::compileShader(const std::string& name, const std::string& source, const std::string& type) {
    return true;
}