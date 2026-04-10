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
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;
    
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    
    HWND hwnd = nullptr;
    int width = 0;
    int height = 0;
    uint32_t currentFrame = 0;
};

class VkInit {
public:
    static bool createInstance(VkInstance& instance);
    static bool createSurface(VkInstance instance, HWND hwnd, VkSurfaceKHR& surface);
    static bool pickPhysicalDevice(VkInstance instance, VkPhysicalDevice& physicalDevice);
    static bool createLogicalDevice(VkPhysicalDevice physicalDevice, VkDevice& device, VkQueue& graphicsQueue);
    static bool createSwapChain(VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface,
                                VkSwapchainKHR& swapChain, VkFormat& imageFormat, VkExtent2D& extent,
                                std::vector<VkImage>& images, int width, int height);
    static bool createImageViews(VkDevice device, const std::vector<VkImage>& images, VkFormat format,
                                 std::vector<VkImageView>& imageViews);
    static bool createRenderPass(VkDevice device, VkFormat format, VkRenderPass& renderPass);
    static bool createGraphicsPipeline(VkDevice device, VkRenderPass renderPass, VkExtent2D extent,
                                       VkPipelineLayout& pipelineLayout, VkPipeline& graphicsPipeline);
    static bool createFramebuffers(VkDevice device, VkRenderPass renderPass,
                                   const std::vector<VkImageView>& imageViews, VkExtent2D extent,
                                   std::vector<VkFramebuffer>& framebuffers);
    static bool createCommandPool(VkDevice device, VkCommandPool& commandPool);
    static bool createVertexBuffer(VkDevice device, VkPhysicalDevice physicalDevice,
                                   VkBuffer& buffer, VkDeviceMemory& memory);
    static bool createCommandBuffers(VkDevice device, VkCommandPool commandPool,
                                     const std::vector<VkFramebuffer>& framebuffers,
                                     std::vector<VkCommandBuffer>& commandBuffers);
    static bool createSyncObjects(VkDevice device, std::vector<VkSemaphore>& imageAvailable,
                                  std::vector<VkSemaphore>& renderFinished, std::vector<VkFence>& fences);
    
    static void cleanupSwapChain(VkDevice device, VkSwapchainKHR swapChain,
                                 std::vector<VkImageView>& imageViews,
                                 std::vector<VkFramebuffer>& framebuffers);
    static void cleanup(VkContext& ctx);
    
private:
    static uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
    static VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code);
    static std::vector<char> readFile(const std::string& filename);
};

#endif