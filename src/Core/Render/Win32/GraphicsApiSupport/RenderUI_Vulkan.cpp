#include "RenderUI_Vulkan.h"
#include <cstring>
#include <cstdio>
#include <vector>
#include <set>
#include <fstream>

RenderUI_Vulkan::RenderUI_Vulkan() 
    : instance(VK_NULL_HANDLE)
    , physicalDevice(VK_NULL_HANDLE)
    , device(VK_NULL_HANDLE)
    , surface(VK_NULL_HANDLE)
    , swapChain(VK_NULL_HANDLE)
    , swapChainImageView(VK_NULL_HANDLE)
    , renderPass(VK_NULL_HANDLE)
    , pipelineLayout(VK_NULL_HANDLE)
    , graphicsPipeline(VK_NULL_HANDLE)
    , commandPool(VK_NULL_HANDLE)
    , commandBuffer(VK_NULL_HANDLE)
    , framebuffer(VK_NULL_HANDLE)
    , vertexBuffer(VK_NULL_HANDLE)
    , vertexBufferMemory(VK_NULL_HANDLE)
    , imageAvailableSemaphore(VK_NULL_HANDLE)
    , renderFinishedSemaphore(VK_NULL_HANDLE)
    , inFlightFence(VK_NULL_HANDLE)
    , fontTexture(0)
    , fontInitialized(false)
    , windowWidth(0)
    , windowHeight(0)
    , hwnd(nullptr)
    , initialized(false)
    , imageIndex(0) {
    
    memset(glyphs, 0, sizeof(glyphs));
}

RenderUI_Vulkan::~RenderUI_Vulkan() {
    cleanup();
}

bool RenderUI_Vulkan::initialize(HWND hwnd, int width, int height) {
    this->hwnd = hwnd;
    this->windowWidth = width;
    this->windowHeight = height;
    
    // Создание Vulkan instance
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "UI Renderer";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Custom Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;
    
    const char* extensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME
    };
    
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = 2;
    createInfo.ppEnabledExtensionNames = extensions;
    
    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        printf("Failed to create Vulkan instance\n");
        return false;
    }
    
    // Выбор физического устройства
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0) return false;
    
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
    physicalDevice = devices[0];
    
    // Создание поверхности
    VkWin32SurfaceCreateInfoKHR surfaceInfo{};
    surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceInfo.hinstance = GetModuleHandle(nullptr);
    surfaceInfo.hwnd = hwnd;
    
    auto createWin32Surface = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR");
    if (!createWin32Surface || createWin32Surface(instance, &surfaceInfo, nullptr, &surface) != VK_SUCCESS) {
        return false;
    }
    
    // Создание логического устройства
    VkDeviceCreateInfo deviceInfo{};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    
    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueInfo{};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = 0;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &queuePriority;
    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pQueueCreateInfos = &queueInfo;
    
    const char* deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    deviceInfo.enabledExtensionCount = 1;
    deviceInfo.ppEnabledExtensionNames = deviceExtensions;
    
    if (vkCreateDevice(physicalDevice, &deviceInfo, nullptr, &device) != VK_SUCCESS) {
        return false;
    }
    
    vkGetDeviceQueue(device, 0, 0, &graphicsQueue);
    
    // Создание swap chain
    VkSwapchainCreateInfoKHR swapInfo{};
    swapInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapInfo.surface = surface;
    swapInfo.minImageCount = 1;
    swapInfo.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
    swapInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    swapInfo.imageExtent = {(uint32_t)width, (uint32_t)height};
    swapInfo.imageArrayLayers = 1;
    swapInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapInfo.clipped = VK_TRUE;
    
    if (vkCreateSwapchainKHR(device, &swapInfo, nullptr, &swapChain) != VK_SUCCESS) {
        return false;
    }
    
    // Получение swap chain images
    uint32_t imageCount;
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    std::vector<VkImage> swapChainImages(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
    
    // Создание image view
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = swapChainImages[0];
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_B8G8R8A8_UNORM;
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    
    if (vkCreateImageView(device, &viewInfo, nullptr, &swapChainImageView) != VK_SUCCESS) {
        return false;
    }
    
    createPipeline();
    initFont();
    createBuffers();
    
    // Создание command pool
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = 0;
    vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool);
    
    // Создание command buffer
    VkCommandBufferAllocateInfo cmdInfo{};
    cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdInfo.commandPool = commandPool;
    cmdInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdInfo.commandBufferCount = 1;
    vkAllocateCommandBuffers(device, &cmdInfo, &commandBuffer);
    
    // Создание semaphores
    VkSemaphoreCreateInfo semInfo{};
    semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    vkCreateSemaphore(device, &semInfo, nullptr, &imageAvailableSemaphore);
    vkCreateSemaphore(device, &semInfo, nullptr, &renderFinishedSemaphore);
    
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    vkCreateFence(device, &fenceInfo, nullptr, &inFlightFence);
    
    initialized = true;
    return true;
}

void RenderUI_Vulkan::cleanup() {
    if (device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device);
        
        if (vertexBuffer) vkDestroyBuffer(device, vertexBuffer, nullptr);
        if (vertexBufferMemory) vkFreeMemory(device, vertexBufferMemory, nullptr);
        
        if (graphicsPipeline) vkDestroyPipeline(device, graphicsPipeline, nullptr);
        if (pipelineLayout) vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        if (renderPass) vkDestroyRenderPass(device, renderPass, nullptr);
        
        if (swapChainImageView) vkDestroyImageView(device, swapChainImageView, nullptr);
        if (swapChain) vkDestroySwapchainKHR(device, swapChain, nullptr);
        
        if (commandPool) vkDestroyCommandPool(device, commandPool, nullptr);
        
        if (imageAvailableSemaphore) vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
        if (renderFinishedSemaphore) vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
        if (inFlightFence) vkDestroyFence(device, inFlightFence, nullptr);
        
        if (surface) vkDestroySurfaceKHR(instance, surface, nullptr);
        if (device) vkDestroyDevice(device, nullptr);
    }
    
    if (instance) vkDestroyInstance(instance, nullptr);
}

void RenderUI_Vulkan::beginFrame() {
    vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &inFlightFence);
    
    vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
}

void RenderUI_Vulkan::endFrame() {
    present();
}

void RenderUI_Vulkan::setup2D(int width, int height) {
    windowWidth = width;
    windowHeight = height;
}

void RenderUI_Vulkan::drawQuad(float x1, float y1, float x2, float y2, float r, float g, float b) {
    std::vector<Vertex> vertices;
    
    float nx1 = (x1 / windowWidth) * 2.0f - 1.0f;
    float ny1 = (y1 / windowHeight) * 2.0f - 1.0f;
    float nx2 = (x2 / windowWidth) * 2.0f - 1.0f;
    float ny2 = (y2 / windowHeight) * 2.0f - 1.0f;
    
    Vertex v0 = {nx1, ny1, r, g, b, 1.0f, 0.0f, 0.0f};
    Vertex v1 = {nx2, ny1, r, g, b, 1.0f, 1.0f, 0.0f};
    Vertex v2 = {nx2, ny2, r, g, b, 1.0f, 1.0f, 1.0f};
    Vertex v3 = {nx1, ny2, r, g, b, 1.0f, 0.0f, 1.0f};
    
    vertices.push_back(v0);
    vertices.push_back(v1);
    vertices.push_back(v2);
    vertices.push_back(v3);
    
    updateVertexBuffer(vertices);
    
    vkResetCommandBuffer(commandBuffer, 0);
    
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = framebuffer;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = {(uint32_t)windowWidth, (uint32_t)windowHeight};
    
    VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;
    
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
    
    VkBuffer vertexBuffers[] = {vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdDraw(commandBuffer, vertices.size(), 1, 0, 0);
    
    vkCmdEndRenderPass(commandBuffer);
    vkEndCommandBuffer(commandBuffer);
}

void RenderUI_Vulkan::drawQuad(int x1, int y1, int x2, int y2, float r, float g, float b) {
    drawQuad((float)x1, (float)y1, (float)x2, (float)y2, r, g, b);
}

void RenderUI_Vulkan::drawText(int x, int y, const std::string& text, float r, float g, float b) {
    // Упрощенная реализация
}

void RenderUI_Vulkan::drawTextCentered(int x, int y, int w, int h, const std::string& text, float r, float g, float b) {
    drawText(x + w/2 - (int)(text.length() * 4), y + h/2 - 8, text, r, g, b);
}

void RenderUI_Vulkan::present() {
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &imageAvailableSemaphore;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderFinishedSemaphore;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.pWaitDstStageMask = waitStages;
    
    vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence);
    
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderFinishedSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapChain;
    presentInfo.pImageIndices = &imageIndex;
    
    vkQueuePresentKHR(graphicsQueue, &presentInfo);
    vkQueueWaitIdle(graphicsQueue);
}

void RenderUI_Vulkan::initFont() {
    fontInitialized = true;
}

void RenderUI_Vulkan::createPipeline() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = VK_FORMAT_B8G8R8A8_UNORM;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    
    vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass);
    
    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    vkCreatePipelineLayout(device, &layoutInfo, nullptr, &pipelineLayout);
    
    VkFramebufferCreateInfo fbInfo{};
    fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbInfo.renderPass = renderPass;
    fbInfo.attachmentCount = 1;
    fbInfo.pAttachments = &swapChainImageView;
    fbInfo.width = windowWidth;
    fbInfo.height = windowHeight;
    fbInfo.layers = 1;
    
    vkCreateFramebuffer(device, &fbInfo, nullptr, &framebuffer);
    
    // Создание pipeline (упрощенно - без шейдеров)
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    
    // Здесь должны быть шейдеры, но для простоты пропускаем
    vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline);
}

void RenderUI_Vulkan::createBuffers() {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = sizeof(Vertex) * 4;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    vkCreateBuffer(device, &bufferInfo, nullptr, &vertexBuffer);
    
    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(device, vertexBuffer, &memReqs);
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = 0;
    
    vkAllocateMemory(device, &allocInfo, nullptr, &vertexBufferMemory);
    vkBindBufferMemory(device, vertexBuffer, vertexBufferMemory, 0);
}

void RenderUI_Vulkan::updateVertexBuffer(const std::vector<Vertex>& vertices) {
    void* data;
    vkMapMemory(device, vertexBufferMemory, 0, sizeof(Vertex) * vertices.size(), 0, &data);
    memcpy(data, vertices.data(), sizeof(Vertex) * vertices.size());
    vkUnmapMemory(device, vertexBufferMemory);
}