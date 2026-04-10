#ifndef SHADER_MANAGER_H
#define SHADER_MANAGER_H

#include <string>

class ShaderManager {
public:
    static ShaderManager& getInstance();
    
    // Берёт строку с шейдером, компилирует и кладёт в папку
    bool compileShader(const std::string& name, const std::string& source, const std::string& type);
    
    // Упрощённый вызов для core
    bool createShaders();

private:
    ShaderManager() = default;
    bool compileGLSL(const std::string& source, const std::string& outputPath, const std::string& type);
};

#define CREATE_SHADERS() ShaderManager::getInstance().createShaders()

#endif