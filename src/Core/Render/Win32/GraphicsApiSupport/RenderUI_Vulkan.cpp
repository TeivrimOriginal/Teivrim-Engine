#include "RenderUI_Vulkan.h"
#include <cstring>
#include <iostream>

#define MAX_FRAMES_IN_FLIGHT 2

RenderUI_Vulkan::RenderUI_Vulkan() 
    : windowWidth(0), windowHeight(0), hwnd(nullptr), initialized(false), currentFrame(0), vertexCount(0) {}

RenderUI_Vulkan::~RenderUI_Vulkan() { cleanup(); }

bool RenderUI_Vulkan::initialize(HWND hwnd, int width, int height) {
    this->hwnd = hwnd;
    this->windowWidth = width;
    this->windowHeight = height;
    this->vk.hwnd = hwnd;
    this->vk.width = width;
    this->vk.height = height;
    
    printf("[VULKAN UI] Initializing...\n");
    
    if (!VkInit::createInstance(vk.instance)) return false;
    if (!VkInit::createSurface(vk.instance, hwnd, vk.surface)) return false;
    if (!VkInit::pickPhysicalDevice(vk.instance, vk.physicalDevice)) return false;
    if (!VkInit::createLogicalDevice(vk.physicalDevice, vk.device, vk.graphicsQueue)) return false;
    if (!VkInit::createSwapChain(vk.physicalDevice, vk.device, vk.surface, vk.swapChain,
                                  vk.swapChainImageFormat, vk.swapChainExtent, vk.swapChainImages, width, height)) return false;
    if (!VkInit::createImageViews(vk.device, vk.swapChainImages, vk.swapChainImageFormat, vk.swapChainImageViews)) return false;
    if (!VkInit::createRenderPass(vk.device, vk.swapChainImageFormat, vk.renderPass)) return false;
    if (!VkInit::createGraphicsPipeline(vk.device, vk.renderPass, vk.swapChainExtent, vk.pipelineLayout, vk.graphicsPipeline)) return false;
    if (!VkInit::createFramebuffers(vk.device, vk.renderPass, vk.swapChainImageViews, vk.swapChainExtent, vk.swapChainFramebuffers)) return false;
    if (!VkInit::createCommandPool(vk.device, vk.commandPool)) return false;
    if (!VkInit::createVertexBuffer(vk.device, vk.physicalDevice, vk.vertexBuffer, vk.vertexBufferMemory)) return false;
    if (!VkInit::createCommandBuffers(vk.device, vk.commandPool, vk.swapChainFramebuffers, vk.commandBuffers)) return false;
    if (!VkInit::createSyncObjects(vk.device, vk.imageAvailableSemaphores, vk.renderFinishedSemaphores, vk.inFlightFences)) return false;
    
    initFont();
    initialized = true;
    printf("[VULKAN UI] Initialization SUCCESS!\n");
    return true;
}

void RenderUI_Vulkan::cleanup() {
    VkInit::cleanup(vk);
    initialized = false;
}

void RenderUI_Vulkan::beginFrame() {}
void RenderUI_Vulkan::endFrame() { present(); }

void RenderUI_Vulkan::setup2D(int width, int height) {
    if (width > 0 && height > 0 && width < 10000 && height < 10000) {
        if (windowWidth != width || windowHeight != height) {
            windowWidth = width;
            windowHeight = height;
        }
    }
}

void RenderUI_Vulkan::drawQuad(float x1, float y1, float x2, float y2, float r, float g, float b) {
    if (windowWidth <= 0 || windowHeight <= 0) return;
    
    float nx1 = (x1 / windowWidth) * 2.0f - 1.0f;
    float ny1 = (y1 / windowHeight) * 2.0f - 1.0f;
    float nx2 = (x2 / windowWidth) * 2.0f - 1.0f;
    float ny2 = (y2 / windowHeight) * 2.0f - 1.0f;
    
    pendingVertices.push_back({nx1, ny1, r, g, b, 1.0f});
    pendingVertices.push_back({nx2, ny1, r, g, b, 1.0f});
    pendingVertices.push_back({nx2, ny2, r, g, b, 1.0f});
    pendingVertices.push_back({nx1, ny1, r, g, b, 1.0f});
    pendingVertices.push_back({nx2, ny2, r, g, b, 1.0f});
    pendingVertices.push_back({nx1, ny2, r, g, b, 1.0f});
}

void RenderUI_Vulkan::drawQuad(int x1, int y1, int x2, int y2, float r, float g, float b) {
    drawQuad((float)x1, (float)y1, (float)x2, (float)y2, r, g, b);
}

void RenderUI_Vulkan::drawText(int x, int y, const std::string& text, float r, float g, float b) {
    float textWidth = text.length() * 8;
    float textHeight = 16;
    drawQuad((float)x, (float)y, (float)(x + textWidth), (float)(y + textHeight), r, g, b);
}

void RenderUI_Vulkan::drawTextCentered(int x, int y, int w, int h, const std::string& text, float r, float g, float b) {
    float textWidth = text.length() * 8;
    float textHeight = 16;
    drawQuad((float)(x + (w - textWidth) / 2), (float)(y + (h - textHeight) / 2),
             (float)(x + (w + textWidth) / 2), (float)(y + (h + textHeight) / 2), r, g, b);
}

void RenderUI_Vulkan::updateVertexBuffer(const std::vector<UIVertex>& vertices) {
    if (vertices.empty()) return;
    void* data;
    vkMapMemory(vk.device, vk.vertexBufferMemory, 0, sizeof(UIVertex) * vertices.size(), 0, &data);
    memcpy(data, vertices.data(), sizeof(UIVertex) * vertices.size());
    vkUnmapMemory(vk.device, vk.vertexBufferMemory);
}

void RenderUI_Vulkan::recreateSwapChain() {
    vkDeviceWaitIdle(vk.device);
    VkInit::cleanupSwapChain(vk.device, vk.swapChain, vk.swapChainImageViews, vk.swapChainFramebuffers);
    VkInit::createSwapChain(vk.physicalDevice, vk.device, vk.surface, vk.swapChain,
                            vk.swapChainImageFormat, vk.swapChainExtent, vk.swapChainImages,
                            windowWidth, windowHeight);
    VkInit::createImageViews(vk.device, vk.swapChainImages, vk.swapChainImageFormat, vk.swapChainImageViews);
    VkInit::createFramebuffers(vk.device, vk.renderPass, vk.swapChainImageViews, vk.swapChainExtent, vk.swapChainFramebuffers);
}

void RenderUI_Vulkan::present() {
    if (!vk.device || !vk.swapChain) return;
    
    if (pendingVertices.size() > 0) {
        updateVertexBuffer(pendingVertices);
        vertexCount = (uint32_t)pendingVertices.size();
        pendingVertices.clear();
    }
    
    vkWaitForFences(vk.device, 1, &vk.inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(vk.device, vk.swapChain, UINT64_MAX,
                                             vk.imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return;
    }
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) return;
    
    vkResetFences(vk.device, 1, &vk.inFlightFences[currentFrame]);
    vkResetCommandBuffer(vk.commandBuffers[imageIndex], 0);
    
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(vk.commandBuffers[imageIndex], &beginInfo);
    
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = vk.renderPass;
    renderPassInfo.framebuffer = vk.swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = vk.swapChainExtent;
    
    VkClearValue clearColor = {{{0.2f, 0.3f, 0.3f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;
    
    vkCmdBeginRenderPass(vk.commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(vk.commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, vk.graphicsPipeline);
    
    if (vertexCount > 0) {
        VkBuffer vertexBuffers[] = {vk.vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(vk.commandBuffers[imageIndex], 0, 1, vertexBuffers, offsets);
        vkCmdDraw(vk.commandBuffers[imageIndex], vertexCount, 1, 0, 0);
    }
    
    vkCmdEndRenderPass(vk.commandBuffers[imageIndex]);
    vkEndCommandBuffer(vk.commandBuffers[imageIndex]);
    
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &vk.imageAvailableSemaphores[currentFrame];
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submitInfo.pWaitDstStageMask = &waitStage;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vk.commandBuffers[imageIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &vk.renderFinishedSemaphores[currentFrame];
    
    vkQueueSubmit(vk.graphicsQueue, 1, &submitInfo, vk.inFlightFences[currentFrame]);
    
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &vk.renderFinishedSemaphores[currentFrame];
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &vk.swapChain;
    presentInfo.pImageIndices = &imageIndex;
    
    vkQueuePresentKHR(vk.graphicsQueue, &presentInfo);
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void RenderUI_Vulkan::initFont() {}