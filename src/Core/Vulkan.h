// Vulkan.h - FULL REFACTORED WITH PROPER LAYERS
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
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
    int width = 0, height = 0;
    bool valid = false;

    VulkanTexture() = default;
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

    void beginFrame();
    void endFrame();
    void present();

    void setup2D(int width, int height);

    void drawBackground(float x1, float y1, float x2, float y2, float r, float g, float b);
    void drawQuad(float x1, float y1, float x2, float y2, float r, float g, float b);
    void drawText(int x, int y, const std::string& text, float r, float g, float b);
    void drawTextCentered(int x, int y, int w, int h, const std::string& text, float r, float g, float b);

    void drawImage(float x1, float y1, float x2, float y2, VulkanTexture* texture);
    void drawImageUV(float x1, float y1, float x2, float y2, VulkanTexture* texture,
                     float u1, float v1, float u2, float v2);

    VulkanTexture* loadUIImage(const std::string& filepath);
    VulkanTexture* loadUIImageFromData(unsigned char* data, int width, int height, int channels);
    void freeUIImage(VulkanTexture* texture);

    void addModel(const std::string& name, const std::vector<StandardMesh>& meshes);
    void removeModel(const std::string& name);
    void clearModels();
    void renderAllModels();
    void renderModel(const std::string& name);
    void setModelTransform(const std::string& name, const glm::mat4& transform);
    void setViewMatrix(const glm::mat4& view);
    void setProjectionMatrix(const glm::mat4& proj);

    void recreateSwapchain();

    VkDevice getDevice() const { return device; }
    bool isWindowValid() const { return hwnd && IsWindow(hwnd); }

    void SetViewportClip(int x, int y, int w, int h);
    void DisableViewportClip();
    bool IsViewportClippingEnabled() const { return viewportClipEnabled; }

    void FlushBackground();

    void renderBackground();
    void renderScene();
    void renderOverlay();

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

    struct ModelBuffers {
        VkBuffer vertexBuffer = VK_NULL_HANDLE;
        VkBuffer indexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
        VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;
        uint32_t indexCount = 0;
        std::vector<VkDescriptorSet> descSets;
        std::vector<VulkanTexture> textures;
    };

    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

    HWND hwnd;
    int width, height;
    bool initialized = false;
    uint32_t currentFrame = 0;
    uint32_t currentImageIndex = 0;
    uint32_t swapchainImageCount = 0;

    glm::mat4 viewMat{1.0f}, projMat{1.0f};

    std::vector<UIQuad> backgroundQuads;
    std::vector<UIQuad> uiQuads;
    std::vector<UITextQuad> uiTextQuads;
    std::vector<UIImageQuad> uiImageQuads;
    std::vector<VulkanTexture*> loadedUITextures;

    std::map<std::string, ModelBuffers> modelBuffers;
    std::map<std::string, glm::mat4> modelTransforms;

    VulkanTexture fontTexture;
    stbtt_bakedchar glyphs[96];
    bool fontInitialized = false;
    std::map<char, CharInfo> charMap;

    VkImage depthImage = VK_NULL_HANDLE;
    VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
    VkImageView depthImageView = VK_NULL_HANDLE;

    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkFormat swapchainFormat;
    VkExtent2D swapchainExtent;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    std::vector<VkFramebuffer> framebuffers;

    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout3D = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayoutUI = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayoutUIText = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayoutUIImage = VK_NULL_HANDLE;

    VkPipeline pipeline3D = VK_NULL_HANDLE;
    VkPipeline pipelineUI = VK_NULL_HANDLE;
    VkPipeline pipelineUIText = VK_NULL_HANDLE;
    VkPipeline pipelineUIImage = VK_NULL_HANDLE;

    VkDescriptorSetLayout descLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout descLayoutUIEmpty = VK_NULL_HANDLE;
    VkDescriptorSetLayout descLayoutUIText = VK_NULL_HANDLE;
    VkDescriptorSetLayout descLayoutUIImage = VK_NULL_HANDLE;
    VkDescriptorPool descPool = VK_NULL_HANDLE;
    VkDescriptorSet descSetUIText = VK_NULL_HANDLE;

    VkCommandPool commandPools[MAX_FRAMES_IN_FLIGHT] = {};
    VkCommandBuffer commandBuffers[MAX_FRAMES_IN_FLIGHT] = {};
    
    VkBuffer uniformBuffers[MAX_FRAMES_IN_FLIGHT] = {};
    VkDeviceMemory uniformBufferMemories[MAX_FRAMES_IN_FLIGHT] = {};
    
    VkSemaphore imageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT] = {};
    VkSemaphore renderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT] = {};
    VkFence inFlightFences[MAX_FRAMES_IN_FLIGHT] = {};

    VkBuffer uiVertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory uiVertexBufferMemory = VK_NULL_HANDLE;

    VkBuffer uiTextVertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory uiTextVertexBufferMemory = VK_NULL_HANDLE;

    VkBuffer uiImageVertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory uiImageVertexBufferMemory = VK_NULL_HANDLE;

    bool viewportClipEnabled = false;
    int clipX = 0, clipY = 0, clipW = 0, clipH = 0;

    void ApplyClipping(float& x1, float& y1, float& x2, float& y2);
    void renderBackgroundImmediate();

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    VkShaderModule createShaderModule(const std::string& filename);
    bool initializeFont();

    VulkanTexture createTextureFromData(unsigned char* data, int width, int height, int channels);
    VulkanTexture createWhiteTexture();

    float getTextWidth(const std::string& text);

    void createMainDescriptorPool();
    void createSwapchain();
    void cleanupSwapchain();
    void createRenderPass();
    void createFramebuffers();
    void createCommandPools();
    void createSyncObjects();
    void createUniformBuffers();
    void createDescriptorSetLayout();
    void createEmptyDescriptorSetLayout();
    void createDescriptorSetLayoutUIText();
    void createDescriptorSetLayoutUIImage();
    void createPipelines();
    void createUIImagePipeline();
    void createUIBuffers();
    void createUITextBuffers();
    void createUIImageBuffers();
    void updateUniformBuffer(uint32_t frameIndex, const glm::mat4& modelMatrix);

    void renderUI();
    void renderUIText();
    void renderUIImage();

    void cleanupTextures();
    void cleanupModelBuffers();

    void createModelBuffers(ModelBuffers& buffers, const std::vector<VertexGPU>& vertices,
                            const std::vector<uint32_t>& indices,
                            const std::vector<VulkanTexture>& textures);
};

#endif