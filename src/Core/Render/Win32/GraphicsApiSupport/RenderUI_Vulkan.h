#ifndef RENDERUI_VULKAN_H
#define RENDERUI_VULKAN_H

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>
#include <windows.h>
#include <string>
#include <vector>
#include "stb_truetype.h"

#define MAX_FRAMES_IN_FLIGHT 2

class RenderUI_Vulkan {
public:
    RenderUI_Vulkan();
    ~RenderUI_Vulkan();
    
    bool initialize(HWND hwnd, int width, int height);
    void cleanup();
    void beginFrame();
    void endFrame();
    void present();
    void setup2D(int width, int height);
    
    void drawQuad(float x1, float y1, float x2, float y2, float r, float g, float b);
    void drawQuad(int x1, int y1, int x2, int y2, float r, float g, float b);
    void drawText(int x, int y, const std::string& text, float r, float g, float b);
    void drawTextCentered(int x, int y, int w, int h, const std::string& text, float r, float g, float b);
    
private:
    struct UIVertex {
        float x, y;
        float r, g, b, a;
    };
    
    bool createInstance();
    bool createSurface();
    bool pickPhysicalDevice();
    bool createLogicalDevice();
    bool createSwapChain();
    bool createImageViews();
    bool createRenderPass();
    bool createGraphicsPipeline();
    bool createFramebuffers();
    bool createCommandPool();
    bool createVertexBuffer();
    bool createCommandBuffers();
    bool createSyncObjects();
    
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                      VkBuffer& buffer, VkDeviceMemory& memory);
    void updateVertexBuffer(const std::vector<UIVertex>& vertices);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    void recreateSwapChain();
    void cleanupSwapChain();
    VkShaderModule createShaderModule(const std::vector<char>& code);
    
    void initFont();
    
    // Vulkan objects
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue graphicsQueue;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapChain;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    
    // Sync objects (ТОЛЬКО ОДИН РАЗ)
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    
    // Font
    unsigned int fontTexture;
    bool fontInitialized;
    stbtt_bakedchar glyphs[96];
    
    int windowWidth;
    int windowHeight;
    HWND hwnd;
    bool initialized;
    uint32_t currentFrame;
    std::vector<UIVertex> pendingVertices;
    uint32_t vertexCount;
};

#endif