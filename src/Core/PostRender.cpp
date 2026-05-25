#include "PostRender.h"
#include "Vulkan.h"
#include <iostream>
#include <vector>
#include <cstring>

void PostRender::Initialize(Vulkan* vulkan, int screenWidth, int screenHeight) {
    std::cout << "[PostRender] Initialize: " << screenWidth << "x" << screenHeight << std::endl;
    m_matrixValid = false;
}

void PostRender::UpdateAndPrintResolution(Vulkan* vulkan, int panelX, int panelY, int panelW, int panelH) {
    static int frameCounter = 0;
    frameCounter++;
    
    if (frameCounter % 60 == 0) {
        std::cout << "[PostRender] Panel resolution: " << panelW << " x " << panelH 
                  << " (X:" << panelX << ", Y:" << panelY << ")" << std::endl;
    }
}

void PostRender::ScanIDMatrix(Vulkan* vulkan) {
    if (!vulkan) {
        std::cerr << "[PostRender] Vulkan is null!" << std::endl;
        return;
    }
    
    int width = vulkan->GetIDBufferWidth();
    int height = vulkan->GetIDBufferHeight();
    
    if (width <= 0 || height <= 0) {
        std::cerr << "[PostRender] ID Buffer not initialized! width=" << width << " height=" << height << std::endl;
        return;
    }
    
    VkDevice device = vulkan->getDevice();
    VkImage idImage = vulkan->GetIDImage();
    VkPhysicalDevice physDevice = vulkan->GetPhysicalDevice();
    
    // Создаём staging buffer
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;
    VkDeviceSize bufferSize = width * height * sizeof(uint32_t);
    
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    
    if (vkCreateBuffer(device, &bufferInfo, nullptr, &stagingBuffer) != VK_SUCCESS) {
        std::cerr << "[PostRender] Failed to create staging buffer" << std::endl;
        return;
    }
    
    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(device, stagingBuffer, &memReq);
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = vulkan->findMemoryType(memReq.memoryTypeBits, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    if (vkAllocateMemory(device, &allocInfo, nullptr, &stagingMemory) != VK_SUCCESS) {
        std::cerr << "[PostRender] Failed to allocate staging memory" << std::endl;
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        return;
    }
    
    vkBindBufferMemory(device, stagingBuffer, stagingMemory, 0);
    
    // Копируем ID изображение в staging buffer
    VkCommandBuffer cmdBuffer = vulkan->beginSingleTimeCommands();
    
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = idImage;
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
    
    vkCmdCopyImageToBuffer(cmdBuffer, idImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           stagingBuffer, 1, &region);
    
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    
    vkCmdPipelineBarrier(cmdBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);
    
    vulkan->endSingleTimeCommands(cmdBuffer);
    
    // Читаем данные
    uint32_t* pixelData = new uint32_t[width * height];
    void* mappedData;
    vkMapMemory(device, stagingMemory, 0, bufferSize, 0, &mappedData);
    memcpy(pixelData, mappedData, bufferSize);
    vkUnmapMemory(device, stagingMemory);
    
    // Заполняем матрицу
    m_idMatrix.clear();
    m_idMatrix.resize(height);
    for (int y = 0; y < height; y++) {
        m_idMatrix[y].resize(width);
        for (int x = 0; x < width; x++) {
            m_idMatrix[y][x] = pixelData[y * width + x];
        }
    }
    
    m_matrixWidth = width;
    m_matrixHeight = height;
    m_matrixValid = true;
    
    // Подсчитываем статистику
    std::map<uint32_t, int> countMap;
    for (int i = 0; i < width * height; i++) {
        countMap[pixelData[i]]++;
    }
    
    std::cout << "[PostRender] ID Matrix scanned: " << width << "x" << height << std::endl;
    std::cout << "[PostRender] Unique IDs: " << countMap.size() << std::endl;
    
    for (auto& pair : countMap) {
        float percent = (float)pair.second / (width * height) * 100.0f;
        if (percent > 0.1f) {
            std::cout << "[PostRender]   ID " << pair.first << ": " << pair.second 
                      << " pixels (" << percent << "%)" << std::endl;
        }
    }
    
    delete[] pixelData;
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingMemory, nullptr);
}

void PostRender::HighlightIDPixels(Vulkan* vulkan, uint32_t targetID, int screenWidth, int screenHeight) {
    if (!vulkan) return;
    
    VkDevice device = vulkan->getDevice();
    VkImage idImage = vulkan->GetIDImage();
    VkPhysicalDevice physDevice = vulkan->GetPhysicalDevice();
    int width = vulkan->GetIDBufferWidth();
    int height = vulkan->GetIDBufferHeight();
    
    if (width <= 0 || height <= 0) return;
    
    // Создаём staging buffer для чтения ID буфера
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;
    VkDeviceSize bufferSize = width * height * sizeof(uint32_t);
    
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    
    if (vkCreateBuffer(device, &bufferInfo, nullptr, &stagingBuffer) != VK_SUCCESS) {
        std::cerr << "[PostRender] Failed to create staging buffer" << std::endl;
        return;
    }
    
    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(device, stagingBuffer, &memReq);
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = vulkan->findMemoryType(memReq.memoryTypeBits, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    if (vkAllocateMemory(device, &allocInfo, nullptr, &stagingMemory) != VK_SUCCESS) {
        std::cerr << "[PostRender] Failed to allocate staging memory" << std::endl;
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        return;
    }
    
    vkBindBufferMemory(device, stagingBuffer, stagingMemory, 0);
    
    // Копируем ID изображение в staging buffer
    VkCommandBuffer cmdBuffer = vulkan->beginSingleTimeCommands();
    
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = idImage;
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
    
    vkCmdCopyImageToBuffer(cmdBuffer, idImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           stagingBuffer, 1, &region);
    
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    
    vkCmdPipelineBarrier(cmdBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);
    
    vulkan->endSingleTimeCommands(cmdBuffer);
    
    // Читаем ID данные
    uint32_t* pixelData = new uint32_t[width * height];
    void* mappedData;
    vkMapMemory(device, stagingMemory, 0, bufferSize, 0, &mappedData);
    memcpy(pixelData, mappedData, bufferSize);
    vkUnmapMemory(device, stagingMemory);
    
    // Анализируем пиксели и рисуем подсветку
    int highlightedPixels = 0;
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            uint32_t id = pixelData[y * width + x];
            
            if (id == targetID) {
                highlightedPixels++;
                
                // Масштабируем координаты ID буфера на экран
                float screenX = (float)x / width * screenWidth;
                float screenY = (float)y / height * screenHeight;
                float pixelW = (float)screenWidth / width;
                float pixelH = (float)screenHeight / height;
                
                // Рисуем красный квадрат на этом пикселе
                vulkan->drawQuad(screenX, screenY, screenX + pixelW, screenY + pixelH, 1.0f, 0.0f, 0.0f);
            }
        }
    }
    
    static int frameCounter = 0;
    frameCounter++;
    if (frameCounter % 60 == 0) {
        std::cout << "[PostRender] ID " << targetID << " found on " << highlightedPixels 
                  << " pixels (" << (float)highlightedPixels / (width * height) * 100.0f << "%)" << std::endl;
    }
    
    delete[] pixelData;
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingMemory, nullptr);
}

void PostRender::HighlightFromMatrix(Vulkan* vulkan, uint32_t targetID, int screenWidth, int screenHeight) {
    if (!vulkan || !m_matrixValid) {
        if (!m_matrixValid) {
            std::cout << "[PostRender] Matrix not valid, call ScanIDMatrix first!" << std::endl;
        }
        return;
    }
    
    if (m_idMatrix.empty()) return;
    
    int height = (int)m_idMatrix.size();
    int width = (int)m_idMatrix[0].size();
    int highlightedPixels = 0;
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (m_idMatrix[y][x] == targetID) {
                highlightedPixels++;
                
                float screenX = (float)x / width * screenWidth;
                float screenY = (float)y / height * screenHeight;
                float pixelW = (float)screenWidth / width;
                float pixelH = (float)screenHeight / height;
                
                vulkan->drawQuad(screenX, screenY, screenX + pixelW, screenY + pixelH, 1.0f, 0.0f, 0.0f);
            }
        }
    }
    
    static int frameCounter = 0;
    frameCounter++;
    if (frameCounter % 60 == 0 && highlightedPixels > 0) {
        std::cout << "[PostRender] From matrix - ID " << targetID << " found on " << highlightedPixels << " pixels" << std::endl;
    }
}

void PostRender::DrawTestSquare(Vulkan* vulkan, int screenWidth, int screenHeight) {
    if (!vulkan) return;
    
    int centerX = screenWidth / 2;
    int centerY = screenHeight / 2;
    int size = 100;
    
    int x1 = centerX - size / 2;
    int y1 = centerY - size / 2;
    int x2 = centerX + size / 2;
    int y2 = centerY + size / 2;
    
    vulkan->drawQuad(x1, y1, x2, y2, 1.0f, 0.0f, 0.0f);
}

uint32_t PostRender::GetIDAt(int x, int y) const {
    if (!m_matrixValid) return 0;
    if (x < 0 || x >= m_matrixWidth || y < 0 || y >= m_matrixHeight) return 0;
    return m_idMatrix[y][x];
}

std::vector<std::pair<int, int>> PostRender::FindPixelsByID(uint32_t targetID) const {
    std::vector<std::pair<int, int>> result;
    
    if (!m_matrixValid) return result;
    
    int height = (int)m_idMatrix.size();
    int width = (int)m_idMatrix[0].size();
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (m_idMatrix[y][x] == targetID) {
                result.push_back({x, y});
            }
        }
    }
    
    return result;
}