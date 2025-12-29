#include "Core/parser.h"
#include "Core/renderer.h"
#include "Core/camera.h"
#include <iostream>
#include <string>
#include "Core/interface.h"

int main() {
    std::cout << "=== 3D MODEL VIEWER ===" << std::endl;
    
    std::string animateInput;
    std::cout << "Enable model rotation animation? (y/n): ";
    std::getline(std::cin, animateInput);
    bool startWithAnimation = (animateInput == "y" || animateInput == "Y" || animateInput == "yes");
    
    Renderer renderer;
    if (!renderer.initialize()) {
        std::cout << "OpenGL initialization failed!" << std::endl;
        return -1;
    }
    
    renderer.setAnimateModel(startWithAnimation);
    std::cout << "Initial animation state: " << (startWithAnimation ? "ENABLED" : "DISABLED") << std::endl;
    std::cout << "OpenGL initialized successfully" << std::endl;
    
    std::cout << "\n=== CAMERA SETTINGS ===" << std::endl;
    std::cout << "View distance: up to 1,000,000 units" << std::endl;
    std::cout << "Near plane: 0.0001 units (very close)" << std::endl;
    std::cout << "This allows viewing vertices at any distance!" << std::endl;
    std::cout << "=======================\n" << std::endl;
    
    GLuint shaderProgram = initShaders();
    if (shaderProgram == 0) {
        std::cout << "Shader compilation failed!" << std::endl;
        return -1;
    }
    std::cout << "Shaders compiled successfully" << std::endl;
    
    std::string filepath;
    std::cout << "\nEnter path to 3D model (FBX/OBJ/etc): ";
    std::getline(std::cin, filepath);
    
    ModelParser parser;
    if (!filepath.empty()) {
        std::cout << "Loading model..." << std::endl;
        if (parser.loadModel(filepath)) {
            const auto& meshes = parser.getMeshes();
            std::cout << "Model loaded successfully!" << std::endl;
            std::cout << "Number of meshes: " << meshes.size() << std::endl;
            
            if (!meshes.empty()) {
                const auto& vertices = meshes[0].vertices;
                if (!vertices.empty()) {
                    float minX = vertices[0].position[0], maxX = vertices[0].position[0];
                    float minY = vertices[0].position[1], maxY = vertices[0].position[1];
                    float minZ = vertices[0].position[2], maxZ = vertices[0].position[2];
                    
                    for (const auto& vertex : vertices) {
                        minX = std::min(minX, vertex.position[0]);
                        maxX = std::max(maxX, vertex.position[0]);
                        minY = std::min(minY, vertex.position[1]);
                        maxY = std::max(maxY, vertex.position[1]);
                        minZ = std::min(minZ, vertex.position[2]);
                        maxZ = std::max(maxZ, vertex.position[2]);
                    }
                    
                    std::cout << "Model bounds:" << std::endl;
                    std::cout << "  X: [" << minX << " to " << maxX << "]" << std::endl;
                    std::cout << "  Y: [" << minY << " to " << maxY << "]" << std::endl;
                    std::cout << "  Z: [" << minZ << " to " << maxZ << "]" << std::endl;
                    
                    float modelSize = std::max({maxX - minX, maxY - minY, maxZ - minZ});
                    std::cout << "  Approximate size: " << modelSize << " units" << std::endl;
                    
                    if (modelSize > 100.0f) {
                        std::cout << "Large model detected! Adjusting camera distance..." << std::endl;
                        glm::vec3 modelCenter((minX + maxX) / 2, (minY + maxY) / 2, (minZ + maxZ) / 2);
                        renderer.getCamera().SetPosition(modelCenter + glm::vec3(0, 0, modelSize * 2));
                    }
                }
            }
        } else {
            std::cout << "Failed to load model: " << filepath << std::endl;
            return -1;
        }
    } else {
        std::cout << "No model specified, running with empty scene" << std::endl;
    }
    
    std::cout << "\n=== FINAL CONTROLS SUMMARY ===" << std::endl;
    std::cout << "MOVEMENT:" << std::endl;
    std::cout << "  WASD - Move camera" << std::endl;
    std::cout << "  Space - Move UP" << std::endl;
    std::cout << "  Ctrl - Move DOWN" << std::endl;
    std::cout << "  Shift (hold) - Sprint (3x speed)" << std::endl;
    std::cout << "  F - Toggle permanent sprint mode" << std::endl;
    std::cout << "\nVIEW:" << std::endl;
    std::cout << "  Mouse - Look around" << std::endl;
    std::cout << "  Mouse Wheel - Zoom" << std::endl;
    std::cout << "  View distance: 1,000,000 units" << std::endl;
    std::cout << "\nMODEL:" << std::endl;
    std::cout << "  R - Toggle model rotation" << std::endl;
    std::cout << "  Current: " << (startWithAnimation ? "ROTATING" : "STATIC") << std::endl;
    std::cout << "\nSYSTEM:" << std::endl;
    std::cout << "  ESC - Exit" << std::endl;
    std::cout << "=================================\n" << std::endl;
    
    std::cout << "Starting main loop..." << std::endl;
    std::cout << "Status updates every 2 seconds in console" << std::endl;
    
    int frameCount = 0;
    
    // СОЗДАЕМ ИНТЕРФЕЙС ПЕРЕД ЦИКЛОМ
    Interface ui;
    ui.initialize(renderer.getWindow());
    
    // ОДИН ЕДИНСТВЕННЫЙ ЦИКЛ РЕНДЕРИНГА
    while (!renderer.shouldClose()) {
        frameCount++;
        
        renderer.beginFrame();
        
        if (!parser.getMeshes().empty()) {
            renderer.renderModel(parser, shaderProgram);
        }
        
        // Рендерим интерфейс поверх 3D
        ui.render();
        
        renderer.endFrame();
    }
    
    // Очистка интерфейса
    ui.cleanup();
    
    glDeleteProgram(shaderProgram);
    
    std::cout << "\n=== APPLICATION STATISTICS ===" << std::endl;
    std::cout << "Total frames rendered: " << frameCount << std::endl;
    std::cout << "Average FPS: " << (frameCount / glfwGetTime()) << std::endl;
    std::cout << "Application closed successfully." << std::endl;
    
    return 0;
}