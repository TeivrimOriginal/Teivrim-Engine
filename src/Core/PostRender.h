// PostRender.h
#ifndef POST_RENDER_H
#define POST_RENDER_H

#include <windows.h>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>

typedef unsigned int ObjectID;

class Vulkan;

struct ScreenQuadVertex {
    float pos[2];
    float uv[2];
};

class PostRender {
public:
    PostRender();
    ~PostRender();
    
    void Initialize(Vulkan* vulkan, int width, int height);
    void Shutdown();
    void Resize(int width, int height);
    
    void Process(Vulkan* vulkan, ObjectID selectedObjectId);
    void Render(Vulkan* vulkan, VkCommandBuffer cmdBuffer, uint32_t screenWidth, uint32_t screenHeight);
    
    void SetOutlineColor(float r, float g, float b);
    void SetOutlineThickness(int thickness);
    void SetEnabled(bool enabled);
    bool IsEnabled() const { return m_enabled; }
    int GetOutlineThickness() const { return m_outlineThickness; }
    
    // Для отладки
    void SetTestMode(bool test) { m_testMode = test; }
    
private:
    bool CreateShaders(Vulkan* vulkan);
    void CreatePipeline(Vulkan* vulkan);
    void CreateVertexBuffer(Vulkan* vulkan);
    
    bool m_initialized = false;
    bool m_enabled = true;
    bool m_testMode = true;  // true - рисует красным все пиксели с ID=99
    int m_width = 0, m_height = 0;
    float m_outlineColor[3] = {1.0f, 0.5f, 0.0f};
    int m_outlineThickness = 3;
    ObjectID m_selectedID = 0;
    
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_descSet = VK_NULL_HANDLE;
    VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_vertexBufferMemory = VK_NULL_HANDLE;
};

#endif