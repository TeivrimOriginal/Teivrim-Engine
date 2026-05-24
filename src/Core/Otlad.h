#ifndef OTLAD_H
#define OTLAD_H

#include <iostream>
#include <string>
#include <atomic>
#include <vector>
#include <map>
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
static std::atomic<bool> g_otladAnalyzeRequested{false};  // Новая команда для анализа ID буфера
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

inline void OtladAnalyze() {
    std::cout << "[OTLAD] Analyze ID Buffer requested" << std::endl;
    g_otladAnalyzeRequested = true;
}

// Функция для анализа ID буфера и вывода статистики по пикселям
inline void OtladAnalyzeIDBuffer(Vulkan* vk) {
    if (!vk) {
        std::cout << "[OTLAD] Vulkan is null!" << std::endl;
        return;
    }
    
    std::cout << "\n========== ID BUFFER ANALYSIS ==========" << std::endl;
    
    // Получаем размеры ID буфера
    int width = vk->GetIDBufferWidth();
    int height = vk->GetIDBufferHeight();
    
    if (width <= 0 || height <= 0) {
        std::cout << "ID Buffer not initialized or invalid size: " << width << "x" << height << std::endl;
        return;
    }
    
    std::cout << "ID Buffer size: " << width << "x" << height << " = " << (width * height) << " pixels" << std::endl;
    
    VkDevice device = vk->getDevice();
    VkPhysicalDevice physDevice = vk->GetPhysicalDevice();
    
    // Создаём staging buffer для чтения данных из GPU
    uint32_t memoryTypeIndex = 0;
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(physDevice, &memProps);
    
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;
    
    VkDeviceSize bufferSize = width * height * sizeof(uint32_t);
    
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    
    if (vkCreateBuffer(device, &bufferInfo, nullptr, &stagingBuffer) != VK_SUCCESS) {
        std::cout << "Failed to create staging buffer" << std::endl;
        return;
    }
    
    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(device, stagingBuffer, &memReq);
    
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
        if ((memReq.memoryTypeBits & (1 << i)) &&
            (memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) &&
            (memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
            memoryTypeIndex = i;
            break;
        }
    }
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;
    
    if (vkAllocateMemory(device, &allocInfo, nullptr, &stagingMemory) != VK_SUCCESS) {
        std::cout << "Failed to allocate staging memory" << std::endl;
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        return;
    }
    
    vkBindBufferMemory(device, stagingBuffer, stagingMemory, 0);
    
    // Копируем ID изображение в staging buffer
    VkCommandBuffer cmdBuffer = vk->beginSingleTimeCommands();
    
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = vk->GetIDImage();
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    
    vkCmdPipelineBarrier(cmdBuffer,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);
    
    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent.width = width;
    region.imageExtent.height = height;
    region.imageExtent.depth = 1;
    
    vkCmdCopyImageToBuffer(cmdBuffer, vk->GetIDImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           stagingBuffer, 1, &region);
    
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    
    vkCmdPipelineBarrier(cmdBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);
    
    vk->endSingleTimeCommands(cmdBuffer);
    
    // Читаем данные из staging buffer
    uint32_t* pixelData = new uint32_t[width * height];
    void* mappedData;
    vkMapMemory(device, stagingMemory, 0, bufferSize, 0, &mappedData);
    memcpy(pixelData, mappedData, bufferSize);
    vkUnmapMemory(device, stagingMemory);
    
    // Анализируем пиксели
    std::map<uint32_t, uint64_t> idCount;
    uint64_t totalPixels = 0;
    uint64_t zeroPixels = 0;
    
    for (int i = 0; i < width * height; i++) {
        uint32_t id = pixelData[i];
        totalPixels++;
        if (id == 0) {
            zeroPixels++;
        } else {
            idCount[id]++;
        }
    }
    
    // Выводим статистику
    std::cout << "\n--- PIXEL STATISTICS ---" << std::endl;
    std::cout << "Total pixels: " << totalPixels << std::endl;
    std::cout << "Pixels with ID=0 (background): " << zeroPixels << " (" 
              << (zeroPixels * 100.0 / totalPixels) << "%)" << std::endl;
    
    std::cout << "\n--- OBJECTS IN ID BUFFER ---" << std::endl;
    if (idCount.empty()) {
        std::cout << "No objects found in ID buffer!" << std::endl;
    } else {
        for (auto& pair : idCount) {
            std::cout << "Object ID: " << pair.first << " -> Pixels: " << pair.second 
                      << " (" << (pair.second * 100.0 / totalPixels) << "%)" << std::endl;
        }
    }
    
    // Выводим примеры пикселей из разных частей экрана (центр и углы)
    std::cout << "\n--- SAMPLE PIXELS ---" << std::endl;
    
    // Центр экрана
    int centerX = width / 2;
    int centerY = height / 2;
    int centerIdx = centerY * width + centerX;
    std::cout << "Center pixel (" << centerX << "," << centerY << ") ID: " << pixelData[centerIdx] << std::endl;
    
    // Углы
    std::cout << "Top-left (0,0) ID: " << pixelData[0] << std::endl;
    std::cout << "Top-right (" << width-1 << ",0) ID: " << pixelData[width-1] << std::endl;
    std::cout << "Bottom-left (0," << height-1 << ") ID: " << pixelData[(height-1) * width] << std::endl;
    std::cout << "Bottom-right (" << width-1 << "," << height-1 << ") ID: " << pixelData[(height-1) * width + (width-1)] << std::endl;
    
    // Несколько случайных пикселей
    std::cout << "\nRandom sample pixels:" << std::endl;
    for (int i = 0; i < 10; i++) {
        int x = (i * 137) % width;
        int y = (i * 271) % height;
        int idx = y * width + x;
        std::cout << "  Pixel (" << x << "," << y << ") ID: " << pixelData[idx] << std::endl;
    }
    
    std::cout << "=========================================\n" << std::endl;
    
    delete[] pixelData;
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingMemory, nullptr);
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
            
            if (obj->parentID != 0) {
                auto* parent = sm.GetSceneObject(obj->parentID);
                if (parent) std::cout << " parent=" << parent->name;
            }
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
    
    // Новая команда для анализа ID буфера
    if (g_otladAnalyzeRequested) {
        g_otladAnalyzeRequested = false;
        OtladAnalyzeIDBuffer(vk);
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