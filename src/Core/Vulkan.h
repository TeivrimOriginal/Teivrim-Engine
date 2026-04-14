#ifndef VULKAN_H
#define VULKAN_H

#include <windows.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "Render/Parser/parser.h"

struct VulkanTexture {
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
    VkSampler sampler;
    int width, height;
    bool valid;
};

class Vulkan {
public:
    Vulkan(HWND hwnd, int width, int height);
    ~Vulkan();
    
    bool isInitialized() const { return initialized; }
    
    void beginFrame();
    void endFrame();
    void present();
    
    void setup2D(int width, int height);
    void drawQuad(float x1, float y1, float x2, float y2, float r, float g, float b);
    void drawText(int x, int y, const std::string& text, float r, float g, float b);
    void drawTextCentered(int x, int y, int w, int h, const std::string& text, float r, float g, float b);
    
    void loadModel(const std::vector<StandardMesh>& meshes);
    void renderModel();
    void setViewMatrix(const glm::mat4& view);
    void setProjectionMatrix(const glm::mat4& proj);
    void setModelMatrix(const glm::mat4& model);
    
    VkDevice getDevice() const { return device; }

private:
    struct VertexGPU { glm::vec3 pos; glm::vec3 color; glm::vec2 texCoord; };
    struct UIVertex { glm::vec2 pos; glm::vec3 color; };
    struct UniformBufferObject { glm::mat4 model; glm::mat4 view; glm::mat4 proj; };
    struct UIQuad { float x1, y1, x2, y2; glm::vec3 color; };
    
    struct FrameResources {
        VkCommandBuffer cmdBuffer;
        VkSemaphore imageAvailableSemaphore;
        VkSemaphore renderFinishedSemaphore;
        VkFence inFlightFence;
        VkBuffer uniformBuffer;
        VkDeviceMemory uniformBufferMemory;
    };
    
    static const int MAX_FRAMES_IN_FLIGHT = 2;
    
    HWND hwnd;
    int width, height;
    bool initialized;
    int currentFrame;
    uint32_t currentImageIndex;
    
    glm::mat4 viewMat, projMat, modelMat;
    
    std::vector<UIQuad> uiQuads;
    std::vector<VertexGPU> modelVertices;
    std::vector<uint32_t> modelIndices;
    std::vector<VulkanTexture> meshTextures;
    std::vector<size_t> meshVertexOffsets;
    std::vector<size_t> meshIndexOffsets;
    bool modelLoaded;
    
    // Vulkan objects (shared)
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
    VkDescriptorSetLayout descLayout;
    VkDescriptorPool descPool;
    std::vector<VkDescriptorSet> descSets;
    
    // Per-frame resources (AAA style)
    FrameResources frames[MAX_FRAMES_IN_FLIGHT];
    
    // Shared buffers
    VkBuffer vertexBuffer, indexBuffer;
    VkDeviceMemory vertexBufferMemory, indexBufferMemory;
    
    VkBuffer uiVertexBuffer;
    VkDeviceMemory uiVertexBufferMemory;
    
    // Methods
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    VkShaderModule createShaderModule(const std::string& filename);
    void createSwapchain();
    void createRenderPass();
    void createFramebuffers();
    void createCommandPool();
    void createSyncObjects();
    void createUniformBuffers();
    void createModelBuffers();
    void createDescriptorSetLayout();
    void createDescriptorPoolAndSets();
    void createPipelines();
    void createUIBuffers();
    void updateUniformBuffer(int frameIndex);
    void renderUI();
    void recreateSwapchain();
    void cleanupTextures();
    void cleanupFrameResources();
    
    VulkanTexture createTextureFromData(unsigned char* data, int width, int height, int channels);
    VulkanTexture createWhiteTexture();
};

#endif