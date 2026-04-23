#ifndef OTLAD_H
#define OTLAD_H

#include <iostream>
#include <string>
#include <atomic>
#include <vector>
#include <windows.h>
#include "Vulkan.h"
#include "../Interface/InterfaceManager.h"
#include "SecondComplexity/Scene/SceneManager.h"

extern InterfaceManager* g_uiManager;

static std::atomic<bool> g_otlad1Requested{false};
static std::atomic<bool> g_otlad2Requested{false};
static std::atomic<bool> g_otladClearRequested{false};
static std::atomic<bool> g_otlad3Requested{false};
static std::atomic<bool> g_otlad4Requested{false};
static VulkanTexture* g_atlasTexture = nullptr;
static bool g_atlasReady = false;

inline void Otlad1() {
    std::cout << "[OTLAD] Command 1 requested" << std::endl;
    g_otlad1Requested = true;
}

inline void Otlad2() {
    std::cout << "[OTLAD] Command 2 requested (render atlas cells)" << std::endl;
    g_otlad2Requested = true;
}

inline void OtladClear() {
    std::cout << "[OTLAD] Clear requested" << std::endl;
    g_otladClearRequested = true;
}

inline void Otlad3() {
    std::cout << "[OTLAD] Scene info requested" << std::endl;
    g_otlad3Requested = true;
}


inline void Otlad4() {
    std::cout << "[OTLAD] Mesh hierarchy scan requested" << std::endl;
    g_otlad4Requested = true;
}
inline void ProcessOtladCommands(Vulkan* vk, InterfaceManager* uiManager) {
    if (!vk || !uiManager) return;
    
    if (g_otladClearRequested) {
        g_otladClearRequested = false;
        uiManager->setTestTexture(nullptr);
        std::cout << "[OTLAD] Cleared" << std::endl;
    }
    
    if (g_otlad1Requested) {
        g_otlad1Requested = false;
        
        char exePath[MAX_PATH];
        GetModuleFileNameA(NULL, exePath, MAX_PATH);
        std::string exeDir = std::string(exePath);
        size_t lastSlash = exeDir.find_last_of("\\");
        if (lastSlash != std::string::npos) {
            exeDir = exeDir.substr(0, lastSlash);
        }
        std::string pngPath = exeDir + "\\1.png";
        
        VulkanTexture* tex = vk->loadUIImage(pngPath);
        if (tex && tex->valid) {
            uiManager->setTestTexture(tex);
            std::cout << "[OTLAD] 1.png loaded!" << std::endl;
        } else {
            std::cerr << "[OTLAD] Failed to load 1.png from: " << pngPath << std::endl;
        }
    }
    
    if (g_otlad2Requested) {
        g_otlad2Requested = false;
        
        char exePath[MAX_PATH];
        GetModuleFileNameA(NULL, exePath, MAX_PATH);
        std::string exeDir = std::string(exePath);
        size_t lastSlash = exeDir.find_last_of("\\");
        if (lastSlash != std::string::npos) {
            exeDir = exeDir.substr(0, lastSlash);
        }
        std::string atlasPath = exeDir + "\\System\\Data\\Interface\\atlas_64.png";
        
        if (g_atlasTexture) {
            vk->freeUIImage(g_atlasTexture);
            g_atlasTexture = nullptr;
        }
        
        g_atlasTexture = vk->loadUIImage(atlasPath);
        
        if (g_atlasTexture && g_atlasTexture->valid) {
            g_atlasReady = true;
            std::cout << "[OTLAD] Atlas loaded: " << g_atlasTexture->width << "x" << g_atlasTexture->height << std::endl;
        } else {
            g_atlasReady = false;
            std::cerr << "[OTLAD] Failed to load atlas_64.png from: " << atlasPath << std::endl;
        }
    }
    
    if (g_otlad3Requested) {
        g_otlad3Requested = false;
        
        std::cout << "\n========== SCENE INFO ==========" << std::endl;
        
        auto& sm = SceneManager::Instance();
        auto objects = sm.GetAllObjects();
        
        std::cout << "Total objects: " << objects.size() << std::endl;
        
        for (auto obj : objects) {
            std::cout << "  " << obj->name 
                      << " [loaded=" << (obj->loaded ? "Y" : "N")
                      << " visible=" << (obj->visible ? "Y" : "N")
                      << " meshes=" << obj->meshCount << "]";
            
            if (obj->parent) std::cout << " parent=" << obj->parent->name;
            if (!obj->modelPath.empty()) std::cout << " path=" << obj->modelPath;
            
            std::cout << std::endl;
        }
        
        std::cout << "===============================\n" << std::endl;
    }
    if (g_otlad4Requested) {
    g_otlad4Requested = false;
    
    std::cout << "\n========== MESH HIERARCHY SCAN ==========" << std::endl;
    
    auto& sm = SceneManager::Instance();
    auto objects = sm.GetAllObjects();
    
    if (objects.empty()) {
        std::cout << "No objects in scene!" << std::endl;
    } else {
        for (auto obj : objects) {
            if (!obj->loaded || !obj->parser) {
                std::cout << "[SKIP] " << obj->name << " (not loaded or no parser)" << std::endl;
                continue;
            }
            
            const auto& meshes = obj->parser->getMeshes();
            std::cout << "\n[OBJECT] " << obj->name << std::endl;
            std::cout << "  Meshes: " << meshes.size() << std::endl;
            std::cout << "  Transform: pos(" << obj->localTransform.position.x << ", " 
                      << obj->localTransform.position.y << ", " << obj->localTransform.position.z << ")" << std::endl;
            
            for (size_t i = 0; i < meshes.size(); i++) {
                const auto& mesh = meshes[i];
                std::cout << "  [MESH " << i << "]" << std::endl;
                std::cout << "    Vertices: " << mesh.vertices.size() << std::endl;
                std::cout << "    Indices: " << mesh.indices.size() << std::endl;
                std::cout << "    Triangles: " << mesh.indices.size() / 3 << std::endl;
                std::cout << "    Textures: " << mesh.textures.size() << std::endl;
                
                if (!mesh.textures.empty()) {
                    for (size_t t = 0; t < mesh.textures.size(); t++) {
                        const auto& tex = mesh.textures[t];
                        std::cout << "      Texture " << t << ": type=" << tex.type;
                        if (tex.rawData.isValid) {
                            std::cout << " size=" << tex.rawData.width << "x" << tex.rawData.height;
                        }
                        std::cout << std::endl;
                    }
                }
                
                if (mesh.vertices.size() > 0) {
                    const auto& v = mesh.vertices[0];
                    std::cout << "    First vertex: pos(" << v.position[0] << ", " 
                              << v.position[1] << ", " << v.position[2] << ") uv(" 
                              << v.texCoords[0] << ", " << v.texCoords[1] << ")" << std::endl;
                }
                
                if (mesh.indices.size() > 0) {
                    std::cout << "    First index: " << mesh.indices[0] << std::endl;
                }
            }
        }
    }
    
    std::cout << "=========================================\n" << std::endl;
}
    
    if (g_atlasReady && g_atlasTexture && g_atlasTexture->valid) {
        int cellSize = 64;
        int spacing = 1;
        int cols = 8;
        int startX = 50, startY = 50;
        float cellUV = 1.0f / cols;
        
        for (int i = 0; i < 64; i++) {
            int row = i / cols;
            int col = i % cols;
            int x = startX + col * (cellSize + spacing);
            int y = startY + row * (cellSize + spacing);
            
            float u1 = col * cellUV;
            float u2 = u1 + cellUV;
            float v1 = row * cellUV;
            float v2 = v1 + cellUV;
            
            float temp = v1;
            v1 = 1.0f - v2;
            v2 = 1.0f - temp;
            
            vk->drawImageUV(x, y, x + cellSize, y + cellSize, g_atlasTexture, u1, v1, u2, v2);
        }
    }
}

#endif