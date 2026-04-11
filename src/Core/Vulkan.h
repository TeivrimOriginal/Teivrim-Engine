#pragma once
#include <windows.h>
#include <vulkan/vulkan.h>
#include <string>
#include <vector>

struct Vertex2D {
    float x, y;
    float r, g, b, a;
};

class Vulkan {
public:
    Vulkan(HWND hwnd, int width, int height);
    ~Vulkan();
    
    void setup2D(int width, int height);
    void beginFrame();
    void endFrame();
    void present();
    
    void drawQuad(float x1, float y1, float x2, float y2, float r, float g, float b);
    void drawQuad(int x1, int y1, int x2, int y2, float r, float g, float b);
    void drawText(int x, int y, const std::string& text, float r, float g, float b);
    void drawTextCentered(int x, int y, int w, int h, const std::string& text, float r, float g, float b);
    
    void drawTriangle();
    
    bool isInitialized() const { return initialized; }
    
private:
    HWND hWnd;
    int windowWidth, windowHeight;
    uint32_t currentImageIndex;
    bool initialized;
    bool recording;
    int frameCount;
    
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
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence fence;
    
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    std::vector<VkFramebuffer> framebuffers;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkShaderModule vertModule, fragModule;
    
    void createInstance();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapchain();
    void createImageViews();
    void createRenderPass();
    void createPipeline();
    void createFramebuffers();
    void createCommandPool();
    void createSemaphores();
    void createVertexBuffer();
    
    void recreateSwapchain();
    void cleanupSwapchain();
    void updateVertexBuffer(const void* data, size_t size);
    
    VkShaderModule createShaderModule(const std::vector<char>& code);
    std::vector<char> readFile(const std::string& filename);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
};