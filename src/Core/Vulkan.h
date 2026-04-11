#pragma once
#include <windows.h>
#include <vulkan/vulkan.h>
#include <string>
#include <vector>

struct Vertex2D {
    float x, y;
    float r, g, b, a;
};

struct Vertex3D {
    float x, y, z;
    float r, g, b;
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
    void set3DData(void* data, int count, int size);
    
    bool isInitialized() const { return initialized; }
    
private:
    HWND hWnd;
    int width, height;
    uint32_t currentImageIndex;
    bool initialized;
    
    // Core Vulkan
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
    
    // 3D Pipeline
    VkRenderPass renderPass3D;
    VkPipelineLayout pipelineLayout3D;
    VkPipeline pipeline3D;
    std::vector<VkFramebuffer> framebuffers3D;
    VkBuffer vertexBuffer3D;
    VkDeviceMemory vertexBufferMemory3D;
    VkShaderModule vertModule3D, fragModule3D;
    uint32_t vertexCount3D;
    size_t vertexSize3D;
    
    // UI Pipeline
    VkRenderPass renderPassUI;
    VkPipelineLayout pipelineLayoutUI;
    VkPipeline pipelineUI;
    std::vector<VkFramebuffer> framebuffersUI;
    VkBuffer vertexBufferUI;
    VkDeviceMemory vertexBufferMemoryUI;
    VkShaderModule vertModuleUI, fragModuleUI;
    
    bool inRenderPass;
    
    void createInstance();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapchain();
    void createImageViews();
    void createCommandPool();
    void createSemaphores();
    void createVertexBuffers();
    
    void createRenderPass3D();
    void createPipeline3D();
    void createFramebuffers3D();
    
    void createRenderPassUI();
    void createPipelineUI();
    void createFramebuffersUI();
    
    void recreateSwapchain();
    void cleanupSwapchain();
    
    void updateUIBuffer(const std::vector<Vertex2D>& vertices);
    void update3DBuffer(const std::vector<Vertex3D>& vertices);
    
    VkShaderModule createShaderModule(const std::string& filename);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    std::vector<char> readFile(const std::string& filename);
};