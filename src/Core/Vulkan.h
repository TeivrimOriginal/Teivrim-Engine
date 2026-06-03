// Vulkan.h - ПОЛНЫЙ ФАЙЛ
#ifndef VULKAN_H
#define VULKAN_H

#include <windows.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <map>
#include <memory>
#include <glm/glm.hpp>
#include "Render/Parser/parser.h"
#include "stb_truetype.h"

constexpr uint32_t MAX_FRAMES_IN_FLIGHT_VK = 2;

class PostRender;

struct VulkanTexture {
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
    int width = 0, height = 0;
    bool valid = false;
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

struct GridVertex {
    glm::vec2 pos;
    glm::vec3 color;
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

    int addModel(const std::string& name, const std::vector<StandardMesh>& meshes);
    void removeModel(const std::string& name);
    void clearModels();
    void renderAllModels();
    void renderModel(const std::string& name);
    void setModelTransform(const std::string& name, const glm::mat4& transform);
    
    void setViewMatrix(const glm::mat4& view);
    void setProjectionMatrix(const glm::mat4& proj);

    void recreateSwapchain();

    VkDevice getDevice() const { return device; }
    VkRenderPass getRenderPass() const { return renderPass; }
    VkCommandBuffer getCurrentCommandBuffer() const { return commandBuffers[currentFrame]; }
    
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer cmdBuffer);
    
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    
    bool isWindowValid() const { return hwnd && IsWindow(hwnd); }

    void SetViewportClip(int x, int y, int w, int h);
    void DisableViewportClip();
    bool IsViewportClippingEnabled() const { return viewportClipEnabled; }

    void FlushBackground();

    void renderBackground();
    void renderScene();
    void renderOverlay();
    
    glm::mat4 getViewMatrix() const { return viewMat; }
    glm::mat4 getProjectionMatrix() const { return projMat; }
    
    void renderGrid(const glm::mat4& viewMatrix, const glm::mat4& projMatrix);
    void setGridEnabled(bool enabled) { gridEnabled = enabled; needGridUpdate = true; }
    void setGridSpacing(float spacing) { gridSpacing = spacing; needGridUpdate = true; }
    void setGridFadeDistance(float distance) { gridFadeDistance = distance; needGridUpdate = true; }
    void setGridLineColor(float r, float g, float b) { 
        gridLineColor[0] = r; gridLineColor[1] = g; gridLineColor[2] = b; 
        needGridUpdate = true; 
    }
    void setGridCenterLineColor(float r, float g, float b) { 
        gridCenterLineColor[0] = r; gridCenterLineColor[1] = g; gridCenterLineColor[2] = b; 
        needGridUpdate = true; 
    }
    bool isGridEnabled() const { return gridEnabled; }
    float getGridSpacing() const { return gridSpacing; }
    
    // Геттеры для PostRender
    VkImage GetIDImage() const { return m_idImage; }
    VkImageView GetIDImageView() const { return m_idImageView; }
    int GetIDBufferWidth() const { return m_idBufferWidth; }
    int GetIDBufferHeight() const { return m_idBufferHeight; }
    VkPhysicalDevice GetPhysicalDevice() const { return physDevice; }
    
    PostRender* GetPostRender() { return m_postRender.get(); }
    
    void InitPostRender(int width, int height);
    void BeginIDPass();
    void RenderModelsToID();
    void EndIDPass();
    
struct VertexGPU {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec3 color;
    glm::vec2 texCoord;
};

struct ModelBuffers {
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;
    uint32_t indexCount = 0;
    std::vector<VkDescriptorSet> descSets;
    std::vector<VulkanTexture> textures;
    
    VkBuffer uniformBuffers[MAX_FRAMES_IN_FLIGHT_VK] = {};
    VkDeviceMemory uniformBufferMemories[MAX_FRAMES_IN_FLIGHT_VK] = {};
};

// Также добавить в public:
VkShaderModule createShaderModule(const std::string& filename);
private:

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
    VkPipelineLayout pipelineLayoutGrid = VK_NULL_HANDLE;

    VkPipeline pipeline3D = VK_NULL_HANDLE;
    VkPipeline pipelineUI = VK_NULL_HANDLE;
    VkPipeline pipelineUIText = VK_NULL_HANDLE;
    VkPipeline pipelineUIImage = VK_NULL_HANDLE;
    VkPipeline pipelineGrid = VK_NULL_HANDLE;

    VkDescriptorSetLayout descLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout descLayoutUIEmpty = VK_NULL_HANDLE;
    VkDescriptorSetLayout descLayoutUIText = VK_NULL_HANDLE;
    VkDescriptorSetLayout descLayoutUIImage = VK_NULL_HANDLE;
    VkDescriptorSetLayout descLayoutGrid = VK_NULL_HANDLE;
    VkDescriptorPool descPool = VK_NULL_HANDLE;
    VkDescriptorSet descSetUIText = VK_NULL_HANDLE;

    VkCommandPool commandPools[MAX_FRAMES_IN_FLIGHT] = {};
    VkCommandBuffer commandBuffers[MAX_FRAMES_IN_FLIGHT] = {};
    
    VkSemaphore imageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT] = {};
    VkSemaphore renderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT] = {};
    VkFence inFlightFences[MAX_FRAMES_IN_FLIGHT] = {};

    VkBuffer uiVertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory uiVertexBufferMemory = VK_NULL_HANDLE;

    VkBuffer uiTextVertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory uiTextVertexBufferMemory = VK_NULL_HANDLE;

    VkBuffer uiImageVertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory uiImageVertexBufferMemory = VK_NULL_HANDLE;

    VkBuffer gridVertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory gridVertexBufferMemory = VK_NULL_HANDLE;
    uint32_t gridVertexCount = 0;

    bool viewportClipEnabled = false;
    int clipX = 0, clipY = 0, clipW = 0, clipH = 0;

    bool gridEnabled = true;
    float gridSpacing = 20.0f;
    float gridFadeDistance = 500.0f;
    float gridYOffset = 0.0f;
    bool needGridUpdate = true;
    float gridLineColor[3] = {0.4f, 0.4f, 0.45f};
    float gridCenterLineColor[3] = {0.8f, 0.8f, 1.0f};
    
    // ID буфер
    int m_idBufferWidth = 0;
    int m_idBufferHeight = 0;
    VkImage m_idImage = VK_NULL_HANDLE;
    VkImageView m_idImageView = VK_NULL_HANDLE;
    VkDeviceMemory m_idImageMemory = VK_NULL_HANDLE;
    VkPipelineLayout m_idPipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_idPipeline = VK_NULL_HANDLE;
    VkRenderPass m_idRenderPass = VK_NULL_HANDLE;
    VkFramebuffer m_idFramebuffer = VK_NULL_HANDLE;
    
    std::unique_ptr<PostRender> m_postRender;

    void ApplyClipping(float& x1, float& y1, float& x2, float& y2);
    void renderBackgroundImmediate();
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
    void createDescriptorSetLayout();
    void createEmptyDescriptorSetLayout();
    void createDescriptorSetLayoutUIText();
    void createDescriptorSetLayoutUIImage();
    void createDescriptorSetLayoutGrid();
    void createPipelines();
    void createGridPipeline();
    void createUIImagePipeline();
    void createUIBuffers();
    void createUITextBuffers();
    void createUIImageBuffers();
    void updateUniformBuffer(ModelBuffers& buffers, uint32_t frameIndex, const glm::mat4& modelMatrix);

    void renderUI();
    void renderUIText();
    void renderUIImage();

    void cleanupTextures();
    void cleanupModelBuffers();

    void createModelBuffers(ModelBuffers& buffers, const std::vector<VertexGPU>& vertices,
                            const std::vector<uint32_t>& indices,
                            const std::vector<VulkanTexture>& textures);
    
    void createPerModelUniformBuffers(ModelBuffers& buffers);
    void destroyPerModelUniformBuffers(ModelBuffers& buffers);
    
    void updateGridBuffer();
};

#endif