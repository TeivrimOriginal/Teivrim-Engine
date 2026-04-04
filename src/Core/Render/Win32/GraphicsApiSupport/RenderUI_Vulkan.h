#ifndef RENDERUI_VULKAN_H
#define RENDERUI_VULKAN_H

#include <vulkan/vulkan.h>
#include <windows.h>
#include <string>
#include <vector>
#include <map>

#include "stb_truetype.h"

class RenderUI_Vulkan {
public:
    RenderUI_Vulkan();
    ~RenderUI_Vulkan();
    
    bool initialize(HWND hwnd, int width, int height);
    void cleanup();
    
    void beginFrame();
    void endFrame();
    void setup2D(int width, int height);
    
    void drawQuad(float x1, float y1, float x2, float y2, float r, float g, float b);
    void drawQuad(int x1, int y1, int x2, int y2, float r, float g, float b);
    
    void drawText(int x, int y, const std::string& text, float r, float g, float b);
    void drawTextCentered(int x, int y, int w, int h, const std::string& text, float r, float g, float b);
    
    void present();

private:
    struct Vertex {
        float x, y;
        float r, g, b, a;
        float u, v;
    };
    
    void initFont();
    void createPipeline();
    void createBuffers();
    void updateVertexBuffer(const std::vector<Vertex>& vertices);
    
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue graphicsQueue;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapChain;
    VkImageView swapChainImageView;
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    VkFramebuffer framebuffer;
    
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;
    
    // Замена GLuint на unsigned int
    unsigned int fontTexture;
    bool fontInitialized;
    stbtt_bakedchar glyphs[96];
    
    int windowWidth;
    int windowHeight;
    HWND hwnd;
    bool initialized;
    uint32_t imageIndex;
};

#endif