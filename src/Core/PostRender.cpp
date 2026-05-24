#include "PostRender.h"
#include "Vulkan.h"
#include <iostream>
#include <vector>

void PostRender::Initialize(Vulkan* vulkan, int screenWidth, int screenHeight) {
    // Ничего не делаем
}

void PostRender::UpdateAndPrintResolution(Vulkan* vulkan, int panelX, int panelY, int panelW, int panelH) {
    static int frameCounter = 0;
    frameCounter++;
    
    if (frameCounter % 60 == 0) {
        std::cout << "[PostRender] Panel resolution: " << panelW << " x " << panelH 
                  << " (X:" << panelX << ", Y:" << panelY << ")" << std::endl;
    }
}

void PostRender::ScanIDMatrix(Vulkan* vulkan, int panelX, int panelY, int panelW, int panelH) {
    if (!vulkan) return;
    
    // Получаем указатель на ID буфер через геттеры
    VkDevice device = vulkan->getDevice();
    VkImage idImage = vulkan->GetIDImage();
    VkPhysicalDevice physDevice = vulkan->GetPhysicalDevice();
    int width = vulkan->GetIDBufferWidth();
    int height = vulkan->GetIDBufferHeight();
    
    if (width <= 0 || height <= 0) return;
    
    // Создаём staging buffer для чтения
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;
    
    VkDeviceSize bufferSize = width * height * sizeof(uint32_t);
    
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    
    if (vkCreateBuffer(device, &bufferInfo, nullptr, &stagingBuffer) != VK_SUCCESS) {
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
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        return;
    }
    
    vkBindBufferMemory(device, stagingBuffer, stagingMemory, 0);
    
    // Копируем ID изображение в staging buffer
    VkCommandBuffer cmdBuffer = vulkan->beginSingleTimeCommands();
    
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
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
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
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
    
    // Выводим статистику каждые 120 кадров
    static int scanCounter = 0;
    scanCounter++;
    if (scanCounter % 120 == 0) {
        std::cout << "[PostRender] ID Matrix scanned: " << width << "x" << height << std::endl;
    }
    
    delete[] pixelData;
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingMemory, nullptr);
}