#ifndef VULKAN_H
#define VULKAN_H

#include <windows.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "Render/Parser/parser.h"

class Vulkan {
public:
    Vulkan(HWND hwnd, int width, int height);
    ~Vulkan();
    
    bool isInitialized() const { return initialized; }
    
    void beginFrame();
    void endFrame();
    void present();
    
    void setup2D(int width, int height);
    
    // UI методы
    void drawQuad(float x1, float y1, float x2, float y2, float r, float g, float b);
    void drawText(int x, int y, const std::string& text, float r, float g, float b);
    void drawTextCentered(int x, int y, int w, int h, const std::string& text, float r, float g, float b);
    
    // 3D методы для модели
    void loadModel(const std::vector<StandardMesh>& meshes);
    void renderModel();
    void setViewMatrix(const glm::mat4& view);
    void setProjectionMatrix(const glm::mat4& proj);
    void setModelMatrix(const glm::mat4& model);
    
    VkDevice getDevice() const { return device; }
    VkPhysicalDevice getPhysicalDevice() const { return physDevice; }
    VkCommandPool getCommandPool() const { return commandPool; }
    VkQueue getGraphicsQueue() const { return graphicsQueue; }

private:
    struct VertexGPU { glm::vec3 pos; glm::vec3 color; glm::vec2 texCoord; };
    struct UIVertex { glm::vec2 pos; glm::vec3 color; };
    struct UniformBufferObject { glm::mat4 model; glm::mat4 view; glm::mat4 proj; };
    struct UIQuad { float x1, y1, x2, y2; glm::vec3 color; };
    
    HWND hwnd;
    int width, height;
    bool initialized;
    float modelAngle;
    glm::mat4 viewMat, projMat, modelMat;
    
    std::vector<UIQuad> uiQuads;
    std::vector<std::tuple<int, int, std::string, glm::vec3>> uiTexts;
    
    // Модель
    std::vector<VertexGPU> modelVertices;
    std::vector<uint32_t> modelIndices;
    bool modelLoaded;
    
    // Vulkan объекты
    VkInstance instance;
    VkPhysicalDevice physDevice;
    VkDevice device;
    VkQueue graphicsQueue;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
    VkFormat swapchainFormat;
    VkExtent2D swapchainExtent;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    std::vector<VkFramebuffer> framebuffers;
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout3D;
    VkPipelineLayout pipelineLayoutUI;
    VkPipeline pipeline3D;
    VkPipeline pipelineUI;
    VkCommandPool commandPool;
    VkCommandBuffer cmdBuffer;
    VkSemaphore imageAvailableSem, renderFinishedSem;
    
    // Буферы для модели
    VkBuffer vertexBuffer, indexBuffer;
    VkDeviceMemory vertexBufferMemory, indexBufferMemory;
    VkBuffer uniformBuffer;
    VkDeviceMemory uniformBufferMemory;
    VkDescriptorSetLayout descLayout;
    VkDescriptorPool descPool;
    VkDescriptorSet descSet;
    
    // Буферы для UI
    VkBuffer uiVertexBuffer;
    VkDeviceMemory uiVertexBufferMemory;
    
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    VkShaderModule createShaderModule(const std::string& filename);
    void createSwapchain();
    void createRenderPass();
    void createFramebuffers();
    void createCommandPool();
    void createSyncObjects();
    void createModelBuffers();
    void createUniformBuffer();
    void createDescriptorSet();
    void createPipelines();
    void createUIBuffers();
    void updateUniformBuffer();
    void renderUI();
    void recreateSwapchain();
};

#endif