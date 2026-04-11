#ifndef VKINIT_H
#define VKINIT_H

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>
#include <windows.h>
#include <vector>
#include <string>

struct VkContext {
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    VkFormat swapChainImageFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D swapChainExtent = {};
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
    VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
    
    VkRenderPass renderPass3D = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout3D = VK_NULL_HANDLE;
    VkPipeline pipeline3D = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> framebuffers3D;
    
    VkRenderPass renderPassUI = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayoutUI = VK_NULL_HANDLE;
    VkPipeline pipelineUI = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> framebuffersUI;
    
    HWND hwnd = nullptr;
    int width = 0;
    int height = 0;
    
    // Данные для рендера
    void* vertexData3D = nullptr;
    uint32_t vertexCount3D = 0;
    size_t vertexSize3D = 0;
    VkBuffer vertexBuffer3D = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory3D = VK_NULL_HANDLE;
    
    void* vertexDataUI = nullptr;
    uint32_t vertexCountUI = 0;
    size_t vertexSizeUI = 0;
    VkBuffer vertexBufferUI = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemoryUI = VK_NULL_HANDLE;
};

class VkInit {
public:
    static VkContext& getContext();
    static bool initialize(HWND hwnd, int width, int height);
    static void cleanup();
    
    static bool beginFrame(uint32_t& imageIndex);
    static void endFrame(uint32_t imageIndex);
    static void renderFrame(); // Один вызов на кадр
    
    static void set3DData(const void* data, uint32_t vertexCount, size_t vertexSize);
    static void setUIData(const void* data, uint32_t vertexCount, size_t vertexSize);
    
private:
    static VkContext ctx;
    static bool initialized;
    static uint32_t currentImageIndex;
    
    static bool createInstance();
    static bool createSurface();
    static bool pickPhysicalDevice();
    static bool createLogicalDevice();
    static bool createSwapChain();
    static bool createImageViews();
    static bool createCommandPool();
    static bool createSemaphores();
    static bool createVertexBuffers();
    
    static bool createRenderPass3D();
    static bool createPipeline3D();
    static bool createFramebuffers3D();
    
    static bool createRenderPassUI();
    static bool createPipelineUI();
    static bool createFramebuffersUI();
    
    static void recreateSwapChain();
    static void cleanupSwapChain();
    
    static uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    static VkShaderModule createShaderModule(const std::vector<char>& code);
    static std::vector<char> readFile(const std::string& filename);
};

#endif