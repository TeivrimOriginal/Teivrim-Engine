#ifndef VULKAN_H
#define VULKAN_H

#include <windows.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <map>
#include <glm/glm.hpp>
#include "Render/Parser/parser.h"
#include "stb_truetype.h"

struct VulkanTexture {
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
    VkSampler sampler;
    int width, height;
    bool valid;
    VulkanTexture() : image(VK_NULL_HANDLE), memory(VK_NULL_HANDLE), view(VK_NULL_HANDLE), 
                      sampler(VK_NULL_HANDLE), width(0), height(0), valid(false) {}
};

struct CharInfo {
    float u1, v1, u2, v2;
    int advance;
    int width, height;
    float xoff, yoff;
};

struct UIImageQuad {
    float x1, y1, x2, y2;
    float u1, v1, u2, v2;
    VulkanTexture* texture;
};

class Vulkan {
public:
    Vulkan(HWND hwnd, int width, int height);
    ~Vulkan();
    
    bool isInitialized() const { return initialized; }
    VulkanTexture* loadUIImageFromData(unsigned char* data, int width, int height, int channels);
    void beginFrame();
    void endFrame();
    void present();
    
    void setup2D(int width, int height);
    void drawQuad(float x1, float y1, float x2, float y2, float r, float g, float b);
    void drawText(int x, int y, const std::string& text, float r, float g, float b);
    void drawTextCentered(int x, int y, int w, int h, const std::string& text, float r, float g, float b);
    
    void drawImage(float x1, float y1, float x2, float y2, VulkanTexture* texture);
    void drawImageUV(float x1, float y1, float x2, float y2, VulkanTexture* texture, 
                     float u1, float v1, float u2, float v2);
    
    VulkanTexture* loadUIImage(const std::string& filepath);
    void freeUIImage(VulkanTexture* texture);
    
    void loadModel(const std::vector<StandardMesh>& meshes);
    void renderModel();
    void setViewMatrix(const glm::mat4& view);
    void setProjectionMatrix(const glm::mat4& proj);
    void setModelMatrix(const glm::mat4& model);
    
    void recreateSwapchain();
    
    VkDevice getDevice() const { return device; }
    bool isWindowValid() const { return hwnd && IsWindow(hwnd); }

private:
    struct VertexGPU { 
        glm::vec3 pos; 
        glm::vec3 color; 
        glm::vec2 texCoord; 
    };
    
    struct UIVertex { 
        glm::vec2 pos; 
        glm::vec3 color; 
    };
    
    struct UITextVertex {
        glm::vec2 pos;
        glm::vec2 texCoord;
        glm::vec3 color;
    };
    
    struct UIImageVertex {
        glm::vec2 pos;
        glm::vec2 texCoord;
    };
    
    struct UniformBufferObject { 
        glm::mat4 model; 
        glm::mat4 view; 
        glm::mat4 proj; 
    };
    
    struct UIQuad { 
        float x1, y1, x2, y2; 
        glm::vec3 color; 
    };
    
    struct UITextQuad {
        float x1, y1, x2, y2;
        float u1, v1, u2, v2;
        glm::vec3 color;
    };
    
    struct FrameData {
        VkCommandPool commandPool;
        VkCommandBuffer cmdBuffer;
        VkSemaphore imageAvailableSemaphore;
        VkSemaphore renderFinishedSemaphore;
        VkFence inFlightFence;
        VkBuffer uniformBuffer;
        VkDeviceMemory uniformBufferMemory;
        
        FrameData() : commandPool(VK_NULL_HANDLE), cmdBuffer(VK_NULL_HANDLE),
                      imageAvailableSemaphore(VK_NULL_HANDLE), renderFinishedSemaphore(VK_NULL_HANDLE),
                      inFlightFence(VK_NULL_HANDLE), uniformBuffer(VK_NULL_HANDLE),
                      uniformBufferMemory(VK_NULL_HANDLE) {}
    };
    
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;
    
    HWND hwnd;
    int width, height;
    bool initialized;
    uint32_t currentFrame;
    uint32_t currentImageIndex;
    uint32_t swapchainImageCount;
    
    glm::mat4 viewMat, projMat, modelMat;
    
    std::vector<UIQuad> uiQuads;
    std::vector<UITextQuad> uiTextQuads;
    std::vector<UIImageQuad> uiImageQuads;
    std::vector<VulkanTexture*> loadedUITextures;
    
    std::vector<VertexGPU> modelVertices;
    std::vector<uint32_t> modelIndices;
    std::vector<VulkanTexture> meshTextures;
    std::vector<size_t> meshVertexOffsets;
    std::vector<size_t> meshIndexOffsets;
    bool modelLoaded;
    
    VulkanTexture fontTexture;
    stbtt_bakedchar glyphs[96];
    bool fontInitialized;
    std::map<char, CharInfo> charMap;
    
    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;
    
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
    VkPipelineLayout pipelineLayoutUIText;
    VkPipelineLayout pipelineLayoutUIImage;
    VkPipeline pipeline3D;
    VkPipeline pipelineUI;
    VkPipeline pipelineUIText;
    VkPipeline pipelineUIImage;
    VkDescriptorSetLayout descLayout;
    VkDescriptorSetLayout descLayoutUIEmpty;
    VkDescriptorSetLayout descLayoutUIText;
    VkDescriptorSetLayout descLayoutUIImage;
    VkDescriptorPool descPool;
    std::vector<VkDescriptorSet> descSets;
    VkDescriptorSet descSetUIText;
    
    std::vector<FrameData> frames;
    
    VkBuffer vertexBuffer;
    VkBuffer indexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkDeviceMemory indexBufferMemory;
    
    VkBuffer uiVertexBuffer;
    VkDeviceMemory uiVertexBufferMemory;
    
    VkBuffer uiTextVertexBuffer;
    VkDeviceMemory uiTextVertexBufferMemory;
    
    VkBuffer uiImageVertexBuffer;
    VkDeviceMemory uiImageVertexBufferMemory;
    
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    VkShaderModule createShaderModule(const std::string& filename);
    void createMainDescriptorPool();
    void createSwapchain();
    void cleanupSwapchain();
    void createRenderPass();
    void createFramebuffers();
    void createCommandPools();
    void createSyncObjects();
    void createUniformBuffers();
    void createModelBuffers();
    void createDescriptorSetLayout();
    void createEmptyDescriptorSetLayout();
    void createDescriptorSetLayoutUIText();
    void createDescriptorSetLayoutUIImage();
    void createDescriptorSetsForModel();
    void createPipelines();
    void createUIImagePipeline();
    void createUIBuffers();
    void createUITextBuffers();
    void createUIImageBuffers();
    void updateUniformBuffer(uint32_t frameIndex);
    void renderUI();
    void renderUIText();
    void renderUIImage();
    void cleanupTextures();
    void cleanupFrameResources();
    void cleanupUITextures();
    
    bool initializeFont();
    VulkanTexture createTextureFromData(unsigned char* data, int width, int height, int channels);
    VulkanTexture createWhiteTexture();
    float getTextWidth(const std::string& text);
};

#endif