#ifndef POST_RENDER_H
#define POST_RENDER_H

#include <vector>
#include <cstdint>
#include <map>
#include <string>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

class Vulkan;

class PostRender {
public:
    PostRender() = default;
    ~PostRender() = default;
    
    void Initialize(Vulkan* vulkan, int screenWidth, int screenHeight);
    void Resize(Vulkan* vulkan, int screenWidth, int screenHeight);
    
    // ID Pass методы
    void BeginIDPass(VkCommandBuffer cmdBuffer, VkRenderPass renderPass, VkFramebuffer framebuffer, int width, int height);
    void EndIDPass(VkCommandBuffer cmdBuffer);
    
    // Обновление и отрисовка подсветки
    void UpdateAndMaybeHighlight(Vulkan* vulkan, int screenWidth, int screenHeight);
    
    // Отрисовка тестового квадрата
    void DrawTestSquare(Vulkan* vulkan, int screenWidth, int screenHeight);
    
    // Геттеры для ID буфера
    VkImage GetIDImage() const { return m_idImage; }
    VkImageView GetIDImageView() const { return m_idImageView; }
    int GetIDBufferWidth() const { return m_matrixWidth; }
    int GetIDBufferHeight() const { return m_matrixHeight; }
    VkRenderPass GetIDRenderPass() const { return m_idRenderPass; }
    VkFramebuffer GetIDFramebuffer() const { return m_idFramebuffer; }
    VkPipelineLayout GetIDPipelineLayout() const { return m_idPipelineLayout; }
    VkPipeline GetIDPipeline() const { return m_idPipeline; }
    
    void Destroy(Vulkan* vulkan);

private:
    void CreateIDBuffer(Vulkan* vulkan, int width, int height);
    void CreateIDPipeline(Vulkan* vulkan);
    void ReadIDBuffer(Vulkan* vulkan);
    
    std::vector<std::vector<uint32_t>> m_idMatrix;
    int m_matrixWidth = 0;
    int m_matrixHeight = 0;
    bool m_matrixValid = false;
    
    VkImage m_idImage = VK_NULL_HANDLE;
    VkImageView m_idImageView = VK_NULL_HANDLE;
    VkDeviceMemory m_idImageMemory = VK_NULL_HANDLE;
    VkPipelineLayout m_idPipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_idPipeline = VK_NULL_HANDLE;
    VkRenderPass m_idRenderPass = VK_NULL_HANDLE;
    VkFramebuffer m_idFramebuffer = VK_NULL_HANDLE;
    
    int m_lastHighlightedID = 3;  // ID для подсветки
    int m_frameCounter = 0;
};

#endif