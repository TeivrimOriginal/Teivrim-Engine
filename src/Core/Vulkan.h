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

    // Методы для UI
    void drawQuad(float x1, float y1, float x2, float y2, float r, float g, float b);
    void drawQuad(int x1, int y1, int x2, int y2, float r, float g, float b);
    void drawText(int x, int y, const std::string& text, float r, float g, float b);
    void drawTextCentered(int x, int y, int w, int h, const std::string& text, float r, float g, float b);

    // 3D
    void drawRotatingCube();

    bool isInitialized() const { return initialized; }

private:
    HWND hWnd;
    int windowWidth = 0, windowHeight = 0;
    uint32_t currentImageIndex = 0;
    bool initialized = false;
    bool recording = false;
    float rotationAngle = 0.0f;

    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkFormat swapchainFormat;
    VkExtent2D swapchainExtent;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    std::vector<VkFramebuffer> framebuffers;

    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

    VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
    VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    VkRenderPass renderPass = VK_NULL_HANDLE;

    // 2D Pipeline (для UI)
    VkPipelineLayout pipelineLayout2D = VK_NULL_HANDLE;
    VkPipeline pipeline2D = VK_NULL_HANDLE;
    VkShaderModule vertModule2D = VK_NULL_HANDLE;
    VkShaderModule fragModule2D = VK_NULL_HANDLE;
    VkBuffer vertexBuffer2D = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory2D = VK_NULL_HANDLE;

    // 3D Pipeline
    VkPipelineLayout pipelineLayout3D = VK_NULL_HANDLE;
    VkPipeline pipeline3D = VK_NULL_HANDLE;
    VkShaderModule vertModule3D = VK_NULL_HANDLE;
    VkShaderModule fragModule3D = VK_NULL_HANDLE;

    VkBuffer vertexBuffer3D = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory3D = VK_NULL_HANDLE;
    VkBuffer indexBuffer3D = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory3D = VK_NULL_HANDLE;

    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    VkBuffer uniformBuffer = VK_NULL_HANDLE;
    VkDeviceMemory uniformBufferMemory = VK_NULL_HANDLE;
    void* uniformBufferMapped = nullptr;

    void createInstance();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapchain();
    void createImageViews();
    void createRenderPass();
    void createPipeline2D();
    void createPipeline3D();
    void createFramebuffers();
    void createCommandPool();
    void createSemaphores();
    void createVertexBuffer2D();
    void createCubeResources();
    void createUniformBuffer();
    void createDescriptorSet();
    void updateUniformBuffer();

    void recreateSwapchain();
    void cleanupSwapchain();
    void updateVertexBuffer2D(const void* data, size_t size);

    VkShaderModule createShaderModule(const std::string& filename);
    std::vector<char> readFile(const std::string& filename);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
};