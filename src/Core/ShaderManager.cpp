#include "ShaderManager.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstdlib>

namespace fs = std::filesystem;

ShaderManager& ShaderManager::getInstance() {
    static ShaderManager instance;
    return instance;
}

bool ShaderManager::compileGLSL(const std::string& source, const std::string& outputPath, const std::string& type) {
    // Временный файл
    std::string tempFile = outputPath + ".tmp";
    std::ofstream temp(tempFile);
    if (!temp.is_open()) return false;
    temp << source;
    temp.close();
    
    // Путь к компилятору
    std::string compiler = "C:/VulkanSDK/1.4.341.1/Bin/glslangValidator.exe";
    if (!fs::exists(compiler)) {
        compiler = "glslangValidator.exe";
    }
    
    std::string cmd = "\"" + compiler + "\" -V \"" + tempFile + "\" -o \"" + outputPath + "\"";
    int result = std::system(cmd.c_str());
    
    fs::remove(tempFile);
    
    if (result != 0) {
        std::cerr << "[ShaderManager] Failed to compile " << outputPath << std::endl;
        return false;
    }
    
    std::cout << "[ShaderManager] Compiled: " << outputPath << std::endl;
    return true;
}

bool ShaderManager::compileShader(const std::string& name, const std::string& source, const std::string& type) {
    std::string outputPath = "src/Poligon/" + name + "." + type + ".spv";
    
    // Проверяем, нужно ли перекомпилировать
    if (fs::exists(outputPath)) {
        return true; // Уже есть
    }
    
    return compileGLSL(source, outputPath, type);
}

bool ShaderManager::createShaders() {
    std::cout << "[ShaderManager] Creating shaders..." << std::endl;
    
    fs::create_directories("src/Poligon");
    
    // Вершинный шейдер для UI
    compileShader("ui_vert", 
        "#version 450\n"
        "layout(location=0) in vec2 pos;\n"
        "layout(location=1) in vec4 col;\n"
        "layout(location=0) out vec4 color;\n"
        "void main() {\n"
        "    gl_Position = vec4(pos, 0.0, 1.0);\n"
        "    color = col;\n"
        "}\n", "vert");
    
    // Фрагментный шейдер для UI
    compileShader("ui_frag",
        "#version 450\n"
        "layout(location=0) in vec4 color;\n"
        "layout(location=0) out vec4 outColor;\n"
        "void main() {\n"
        "    outColor = color;\n"
        "}\n", "frag");
    
    return true;
}