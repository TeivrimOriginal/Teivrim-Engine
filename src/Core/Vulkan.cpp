#include "Vulkan.h"
#include <iostream>
#include <fstream>
#include <cstring>

std::vector<char> Vulkan::readFile(const std::string& filename) {
    printf("[Vulkan DEBUG] readFile: trying to open '%s'\n", filename.c_str());
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        printf("[Vulkan ERROR] Failed to open: %s\n", filename.c_str());
        return {};
    }
    size_t size = file.tellg();
    printf("[Vulkan DEBUG] readFile: '%s' size=%zu bytes\n", filename.c_str(), size);
    std::vector<char> buffer(size);
    file.seekg(0);
    file.read(buffer.data(), size);
    file.close();
    return buffer;
}

Vulkan::Vulkan(HWND hwnd, int w, int h) 
    : hWnd(hwnd), windowWidth(w), windowHeight(h), currentImageIndex(0), 
      initialized(false), recording(false), frameCount(0),
      instance(VK_NULL_HANDLE), physDevice(VK_NULL_HANDLE), device(VK_NULL_HANDLE),
      surface(VK_NULL_HANDLE), swapchain(VK_NULL_HANDLE), commandPool(VK_NULL_HANDLE),
      commandBuffer(VK_NULL_HANDLE), imageAvailableSemaphore(VK_NULL_HANDLE),
      renderFinishedSemaphore(VK_NULL_HANDLE), fence(VK_NULL_HANDLE),
      renderPass(VK_NULL_HANDLE), pipelineLayout(VK_NULL_HANDLE), pipeline(VK_NULL_HANDLE),
      vertexBuffer(VK_NULL_HANDLE), vertexBufferMemory(VK_NULL_HANDLE),
      vertModule(VK_NULL_HANDLE), fragModule(VK_NULL_HANDLE) {
    
    printf("[Vulkan] Initializing...\n");
    printf("[Vulkan DEBUG] Window size: %dx%d\n", windowWidth, windowHeight);
    
    createInstance();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapchain();
    createImageViews();
    createRenderPass();
    createPipeline();
    createFramebuffers();
    createCommandPool();
    createSemaphores();
    createVertexBuffer();
    
    VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    VkResult fenceResult = vkCreateFence(device, &fenceInfo, nullptr, &fence);
    printf("[Vulkan DEBUG] vkCreateFence result: %d\n", fenceResult);
    
    initialized = true;
    printf("[Vulkan] Initialized successfully!\n");
}

Vulkan::~Vulkan() {
    if (!initialized) return;
    vkDeviceWaitIdle(device);
    
    cleanupSwapchain();
    
    if (fence) vkDestroyFence(device, fence, nullptr);
    if (pipeline) vkDestroyPipeline(device, pipeline, nullptr);
    if (pipelineLayout) vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    if (renderPass) vkDestroyRenderPass(device, renderPass, nullptr);
    if (vertModule) vkDestroyShaderModule(device, vertModule, nullptr);
    if (fragModule) vkDestroyShaderModule(device, fragModule, nullptr);
    if (vertexBuffer) vkDestroyBuffer(device, vertexBuffer, nullptr);
    if (vertexBufferMemory) vkFreeMemory(device, vertexBufferMemory, nullptr);
    if (commandPool) vkDestroyCommandPool(device, commandPool, nullptr);
    if (imageAvailableSemaphore) vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
    if (renderFinishedSemaphore) vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
    if (device) vkDestroyDevice(device, nullptr);
    if (surface) vkDestroySurfaceKHR(instance, surface, nullptr);
    if (instance) vkDestroyInstance(instance, nullptr);
    
    printf("[Vulkan] Cleanup complete\n");
}

void Vulkan::setup2D(int w, int h) {
    windowWidth = w;
    windowHeight = h;
    printf("[Vulkan DEBUG] setup2D: %dx%d\n", w, h);
}

void Vulkan::beginFrame() {
    if (!initialized) {
        printf("[Vulkan ERROR] beginFrame: not initialized!\n");
        return;
    }
    
    printf("[Vulkan DEBUG] beginFrame frame=%d\n", ++frameCount);
    
    VkResult waitResult = vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
    printf("[Vulkan DEBUG] vkWaitForFences result: %d\n", waitResult);
    
    vkResetFences(device, 1, &fence);
    
    VkResult acquireResult = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, 
                                             imageAvailableSemaphore, VK_NULL_HANDLE, &currentImageIndex);
    printf("[Vulkan DEBUG] vkAcquireNextImageKHR result: %d, imageIndex: %u\n", acquireResult, currentImageIndex);
    
    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
        printf("[Vulkan DEBUG] Swapchain out of date, recreating...\n");
        recreateSwapchain();
        return;
    }
    
    vkResetCommandBuffer(commandBuffer, 0);
    
    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    VkResult beginResult = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    printf("[Vulkan DEBUG] vkBeginCommandBuffer result: %d\n", beginResult);
    recording = true;
}

void Vulkan::endFrame() {
    if (!initialized || !recording) {
        printf("[Vulkan DEBUG] endFrame: skip (initialized=%d, recording=%d)\n", initialized, recording);
        return;
    }
    
    printf("[Vulkan DEBUG] endFrame\n");
    
    VkResult endResult = vkEndCommandBuffer(commandBuffer);
    printf("[Vulkan DEBUG] vkEndCommandBuffer result: %d\n", endResult);
    recording = false;
    
    VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &imageAvailableSemaphore;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderFinishedSemaphore;
    
    VkResult submitResult = vkQueueSubmit(graphicsQueue, 1, &submitInfo, fence);
    printf("[Vulkan DEBUG] vkQueueSubmit result: %d\n", submitResult);
}

void Vulkan::present() {
    if (!initialized) return;
    
    printf("[Vulkan DEBUG] present frame=%d, imageIndex=%u\n", frameCount, currentImageIndex);
    
    VkPresentInfoKHR presentInfo{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderFinishedSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &currentImageIndex;
    
    VkResult presentResult = vkQueuePresentKHR(graphicsQueue, &presentInfo);
    printf("[Vulkan DEBUG] vkQueuePresentKHR result: %d\n", presentResult);
    
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR) {
        printf("[Vulkan DEBUG] Present out of date, recreating swapchain...\n");
        recreateSwapchain();
    }
}

void Vulkan::drawQuad(float x1, float y1, float x2, float y2, float r, float g, float b) {
    if (!initialized || !recording) {
        printf("[Vulkan DEBUG] drawQuad skip: initialized=%d, recording=%d\n", initialized, recording);
        return;
    }
    
    printf("[Vulkan DEBUG] drawQuad: (%.0f,%.0f)-(%.0f,%.0f) color(%.2f,%.2f,%.2f)\n", x1, y1, x2, y2, r, g, b);
    
    float nx1 = (x1 / windowWidth) * 2.0f - 1.0f;
    float ny1 = ((windowHeight - y1) / windowHeight) * 2.0f - 1.0f;
    float nx2 = (x2 / windowWidth) * 2.0f - 1.0f;
    float ny2 = ((windowHeight - y2) / windowHeight) * 2.0f - 1.0f;
    
    struct Vertex { float x, y; float r, g, b, a; };
    Vertex vertices[6] = {
        {nx1, ny1, r, g, b, 1.0f},
        {nx2, ny1, r, g, b, 1.0f},
        {nx2, ny2, r, g, b, 1.0f},
        {nx1, ny1, r, g, b, 1.0f},
        {nx2, ny2, r, g, b, 1.0f},
        {nx1, ny2, r, g, b, 1.0f}
    };
    
    updateVertexBuffer(vertices, sizeof(vertices));
    
    VkRenderPassBeginInfo rpBegin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    rpBegin.renderPass = renderPass;
    rpBegin.framebuffer = framebuffers[currentImageIndex];
    rpBegin.renderArea.offset = {0, 0};
    rpBegin.renderArea.extent = {uint32_t(windowWidth), uint32_t(windowHeight)};
    VkClearValue clearColor = {0.1f, 0.1f, 0.2f, 1.0f};
    rpBegin.clearValueCount = 1;
    rpBegin.pClearValues = &clearColor;
    
    vkCmdBeginRenderPass(commandBuffer, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    VkBuffer vertexBuffers[] = {vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdDraw(commandBuffer, 6, 1, 0, 0);
    vkCmdEndRenderPass(commandBuffer);
}

void Vulkan::drawQuad(int x1, int y1, int x2, int y2, float r, float g, float b) {
    drawQuad((float)x1, (float)y1, (float)x2, (float)y2, r, g, b);
}

void Vulkan::drawText(int x, int y, const std::string& text, float r, float g, float b) {
    for (size_t i = 0; i < text.size(); i++) {
        drawQuad(x + i * 10, y, x + (i + 1) * 10, y + 16, r, g, b);
    }
}

void Vulkan::drawTextCentered(int x, int y, int w, int h, const std::string& text, float r, float g, float b) {
    int textWidth = (int)text.size() * 10;
    int startX = x + (w - textWidth) / 2;
    int startY = y + (h - 16) / 2;
    drawText(startX, startY, text, r, g, b);
}

void Vulkan::drawTriangle() {
    if (!initialized || !recording) {
        printf("[Vulkan DEBUG] drawTriangle skip: initialized=%d, recording=%d\n", initialized, recording);
        return;
    }
    
    printf("[Vulkan DEBUG] drawTriangle\n");
    
    struct Vertex { float x, y; float r, g, b, a; };
    Vertex vertices[3] = {
        {0.0f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f},
        {0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f},
        {-0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f}
    };
    
    updateVertexBuffer(vertices, sizeof(vertices));
    
    VkRenderPassBeginInfo rpBegin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    rpBegin.renderPass = renderPass;
    rpBegin.framebuffer = framebuffers[currentImageIndex];
    rpBegin.renderArea.offset = {0, 0};
    rpBegin.renderArea.extent = {uint32_t(windowWidth), uint32_t(windowHeight)};
    VkClearValue clearColor = {0.1f, 0.1f, 0.2f, 1.0f};
    rpBegin.clearValueCount = 1;
    rpBegin.pClearValues = &clearColor;
    
    vkCmdBeginRenderPass(commandBuffer, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    VkBuffer vertexBuffers[] = {vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    vkCmdEndRenderPass(commandBuffer);
}

// ==================== PRIVATE METHODS ====================

void Vulkan::createInstance() {
    printf("[Vulkan DEBUG] createInstance\n");
    VkApplicationInfo appInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    appInfo.apiVersion = VK_API_VERSION_1_0;
    
    const char* extensions[] = {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME};
    VkInstanceCreateInfo instInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instInfo.pApplicationInfo = &appInfo;
    instInfo.enabledExtensionCount = 2;
    instInfo.ppEnabledExtensionNames = extensions;
    VkResult result = vkCreateInstance(&instInfo, nullptr, &instance);
    printf("[Vulkan DEBUG] vkCreateInstance result: %d\n", result);
}

void Vulkan::createSurface() {
    printf("[Vulkan DEBUG] createSurface\n");
    VkWin32SurfaceCreateInfoKHR win32Info{VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
    win32Info.hinstance = GetModuleHandle(nullptr);
    win32Info.hwnd = hWnd;
    VkResult result = vkCreateWin32SurfaceKHR(instance, &win32Info, nullptr, &surface);
    printf("[Vulkan DEBUG] vkCreateWin32SurfaceKHR result: %d\n", result);
}

void Vulkan::pickPhysicalDevice() {
    printf("[Vulkan DEBUG] pickPhysicalDevice\n");
    uint32_t deviceCount;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    printf("[Vulkan DEBUG] deviceCount: %u\n", deviceCount);
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
    physDevice = devices[0];
    
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(physDevice, &props);
    printf("[Vulkan] GPU: %s\n", props.deviceName);
}

void Vulkan::createLogicalDevice() {
    printf("[Vulkan DEBUG] createLogicalDevice\n");
    uint32_t queueFamily = 0;
    VkDeviceQueueCreateInfo queueInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queueInfo.queueFamilyIndex = queueFamily;
    queueInfo.queueCount = 1;
    float priority = 1.0f;
    queueInfo.pQueuePriorities = &priority;
    
    const char* devExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    VkDeviceCreateInfo devInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    devInfo.queueCreateInfoCount = 1;
    devInfo.pQueueCreateInfos = &queueInfo;
    devInfo.enabledExtensionCount = 1;
    devInfo.ppEnabledExtensionNames = devExtensions;
    VkResult result = vkCreateDevice(physDevice, &devInfo, nullptr, &device);
    printf("[Vulkan DEBUG] vkCreateDevice result: %d\n", result);
    vkGetDeviceQueue(device, queueFamily, 0, &graphicsQueue);
}

void Vulkan::createSwapchain() {
    printf("[Vulkan DEBUG] createSwapchain: windowSize=%dx%d\n", windowWidth, windowHeight);
    
    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDevice, surface, &caps);
    printf("[Vulkan DEBUG] caps: minImageCount=%u, maxImageCount=%u\n", caps.minImageCount, caps.maxImageCount);
    printf("[Vulkan DEBUG] caps: currentExtent=%dx%d\n", caps.currentExtent.width, caps.currentExtent.height);
    printf("[Vulkan DEBUG] caps: minImageExtent=%dx%d\n", caps.minImageExtent.width, caps.minImageExtent.height);
    printf("[Vulkan DEBUG] caps: maxImageExtent=%dx%d\n", caps.maxImageExtent.width, caps.maxImageExtent.height);
    
    uint32_t minImageCount = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && minImageCount > caps.maxImageCount) {
        minImageCount = caps.maxImageCount;
    }
    printf("[Vulkan DEBUG] final minImageCount: %u\n", minImageCount);
    
    VkSurfaceTransformFlagBitsKHR preTransform = caps.currentTransform;
    if (!(caps.supportedTransforms & preTransform)) {
        preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }
    printf("[Vulkan DEBUG] preTransform: %u\n", preTransform);
    
    VkExtent2D extent = caps.currentExtent;
    if (extent.width == 0xFFFFFFFF) {
        extent.width = windowWidth;
        extent.height = windowHeight;
    }
    printf("[Vulkan DEBUG] final extent: %dx%d\n", extent.width, extent.height);
    
    VkSurfaceFormatKHR format{VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    swapchainFormat = format.format;
    swapchainExtent = extent;
    
    VkSwapchainCreateInfoKHR swapInfo{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    swapInfo.surface = surface;
    swapInfo.minImageCount = minImageCount;
    swapInfo.imageFormat = format.format;
    swapInfo.imageColorSpace = format.colorSpace;
    swapInfo.imageExtent = extent;
    swapInfo.imageArrayLayers = 1;
    swapInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapInfo.preTransform = preTransform;
    swapInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapInfo.clipped = VK_TRUE;
    
    VkResult result = vkCreateSwapchainKHR(device, &swapInfo, nullptr, &swapchain);
    printf("[Vulkan DEBUG] vkCreateSwapchainKHR result: %d\n", result);
    
    uint32_t imageCount;
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
    swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data());
    printf("[Vulkan DEBUG] swapchain imageCount: %u\n", imageCount);
}

void Vulkan::createImageViews() {
    printf("[Vulkan DEBUG] createImageViews, imageCount=%zu\n", swapchainImages.size());
    swapchainImageViews.resize(swapchainImages.size());
    for (size_t i = 0; i < swapchainImages.size(); i++) {
        VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        viewInfo.image = swapchainImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = swapchainFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;
        VkResult result = vkCreateImageView(device, &viewInfo, nullptr, &swapchainImageViews[i]);
        if (result != VK_SUCCESS) {
            printf("[Vulkan ERROR] vkCreateImageView %zu failed: %d\n", i, result);
        }
    }
}

void Vulkan::createRenderPass() {
    printf("[Vulkan DEBUG] createRenderPass\n");
    VkAttachmentDescription colorAtt{};
    colorAtt.format = swapchainFormat;
    colorAtt.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAtt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAtt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAtt.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    
    VkAttachmentReference colorRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;
    
    VkRenderPassCreateInfo rpInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    rpInfo.attachmentCount = 1;
    rpInfo.pAttachments = &colorAtt;
    rpInfo.subpassCount = 1;
    rpInfo.pSubpasses = &subpass;
    VkResult result = vkCreateRenderPass(device, &rpInfo, nullptr, &renderPass);
    printf("[Vulkan DEBUG] vkCreateRenderPass result: %d\n", result);
}

void Vulkan::createPipeline() {
    printf("[Vulkan DEBUG] createPipeline\n");
    
    auto vertCode = readFile("vert.spv");
    auto fragCode = readFile("frag.spv");
    
    if (vertCode.empty() || fragCode.empty()) {
        printf("[Vulkan ERROR] Shaders not found! vertSize=%zu, fragSize=%zu\n", vertCode.size(), fragCode.size());
        return;
    }
    
    vertModule = createShaderModule(vertCode);
    fragModule = createShaderModule(fragCode);
    
    if (vertModule == VK_NULL_HANDLE || fragModule == VK_NULL_HANDLE) {
        printf("[Vulkan ERROR] Failed to create shader modules!\n");
        return;
    }
    printf("[Vulkan DEBUG] Shader modules created successfully\n");
    
    VkPipelineShaderStageCreateInfo stages[2] = {};
    stages[0] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertModule;
    stages[0].pName = "main";
    stages[1] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragModule;
    stages[1].pName = "main";
    
    VkVertexInputBindingDescription bindingDesc{};
    bindingDesc.binding = 0;
    bindingDesc.stride = sizeof(float) * 6;
    bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    VkVertexInputAttributeDescription attribDesc[2]{};
    attribDesc[0].binding = 0;
    attribDesc[0].location = 0;
    attribDesc[0].format = VK_FORMAT_R32G32_SFLOAT;
    attribDesc[0].offset = 0;
    attribDesc[1].binding = 0;
    attribDesc[1].location = 1;
    attribDesc[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attribDesc[1].offset = sizeof(float) * 2;
    
    VkPipelineVertexInputStateCreateInfo vertexInput{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &bindingDesc;
    vertexInput.vertexAttributeDescriptionCount = 2;
    vertexInput.pVertexAttributeDescriptions = attribDesc;
    
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    
    VkViewport viewport{0, 0, (float)windowWidth, (float)windowHeight, 0, 1};
    VkRect2D scissor{{0,0}, {uint32_t(windowWidth), uint32_t(windowHeight)}};
    VkPipelineViewportStateCreateInfo viewportState{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;
    
    VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.cullMode = VK_CULL_MODE_NONE;
    raster.lineWidth = 1.0f;
    
    VkPipelineMultisampleStateCreateInfo multisample{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    
    VkPipelineColorBlendAttachmentState blendAtt{};
    blendAtt.blendEnable = VK_TRUE;
    blendAtt.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendAtt.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAtt.colorBlendOp = VK_BLEND_OP_ADD;
    blendAtt.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blendAtt.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blendAtt.alphaBlendOp = VK_BLEND_OP_ADD;
    blendAtt.colorWriteMask = 0xF;
    
    VkPipelineColorBlendStateCreateInfo colorBlend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    colorBlend.attachmentCount = 1;
    colorBlend.pAttachments = &blendAtt;
    
    VkPipelineLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    VkResult layoutResult = vkCreatePipelineLayout(device, &layoutInfo, nullptr, &pipelineLayout);
    printf("[Vulkan DEBUG] vkCreatePipelineLayout result: %d\n", layoutResult);
    
    VkGraphicsPipelineCreateInfo pipelineInfo{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = stages;
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &raster;
    pipelineInfo.pMultisampleState = &multisample;
    pipelineInfo.pColorBlendState = &colorBlend;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    
    VkResult pipelineResult = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline);
    printf("[Vulkan DEBUG] vkCreateGraphicsPipelines result: %d\n", pipelineResult);
}

void Vulkan::createFramebuffers() {
    printf("[Vulkan DEBUG] createFramebuffers, imageCount=%zu, size=%dx%d\n", 
           swapchainImageViews.size(), windowWidth, windowHeight);
    framebuffers.resize(swapchainImageViews.size());
    for (size_t i = 0; i < swapchainImageViews.size(); i++) {
        VkFramebufferCreateInfo fbInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        fbInfo.renderPass = renderPass;
        fbInfo.attachmentCount = 1;
        fbInfo.pAttachments = &swapchainImageViews[i];
        fbInfo.width = uint32_t(windowWidth);
        fbInfo.height = uint32_t(windowHeight);
        fbInfo.layers = 1;
        VkResult result = vkCreateFramebuffer(device, &fbInfo, nullptr, &framebuffers[i]);
        if (result != VK_SUCCESS) {
            printf("[Vulkan ERROR] vkCreateFramebuffer %zu failed: %d\n", i, result);
        }
    }
}

void Vulkan::createCommandPool() {
    printf("[Vulkan DEBUG] createCommandPool\n");
    VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VkResult poolResult = vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool);
    printf("[Vulkan DEBUG] vkCreateCommandPool result: %d\n", poolResult);
    
    VkCommandBufferAllocateInfo allocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    VkResult allocResult = vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
    printf("[Vulkan DEBUG] vkAllocateCommandBuffers result: %d\n", allocResult);
}

void Vulkan::createSemaphores() {
    printf("[Vulkan DEBUG] createSemaphores\n");
    VkSemaphoreCreateInfo semInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkResult result1 = vkCreateSemaphore(device, &semInfo, nullptr, &imageAvailableSemaphore);
    VkResult result2 = vkCreateSemaphore(device, &semInfo, nullptr, &renderFinishedSemaphore);
    printf("[Vulkan DEBUG] vkCreateSemaphore results: %d, %d\n", result1, result2);
}

void Vulkan::createVertexBuffer() {
    printf("[Vulkan DEBUG] createVertexBuffer\n");
    VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = 1024 * 1024;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkResult bufferResult = vkCreateBuffer(device, &bufferInfo, nullptr, &vertexBuffer);
    printf("[Vulkan DEBUG] vkCreateBuffer result: %d\n", bufferResult);
    
    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(device, vertexBuffer, &memReqs);
    printf("[Vulkan DEBUG] buffer memory requirements: size=%llu, alignment=%llu\n", memReqs.size, memReqs.alignment);
    
    VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, 
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    VkResult memResult = vkAllocateMemory(device, &allocInfo, nullptr, &vertexBufferMemory);
    printf("[Vulkan DEBUG] vkAllocateMemory result: %d, memoryTypeIndex=%u\n", memResult, allocInfo.memoryTypeIndex);
    
    vkBindBufferMemory(device, vertexBuffer, vertexBufferMemory, 0);
}

void Vulkan::updateVertexBuffer(const void* data, size_t size) {
    void* mapped;
    vkMapMemory(device, vertexBufferMemory, 0, size, 0, &mapped);
    memcpy(mapped, data, size);
    vkUnmapMemory(device, vertexBufferMemory);
}

void Vulkan::recreateSwapchain() {
    printf("[Vulkan DEBUG] recreateSwapchain\n");
    vkDeviceWaitIdle(device);
    
    RECT rect;
    GetClientRect(hWnd, &rect);
    windowWidth = rect.right - rect.left;
    windowHeight = rect.bottom - rect.top;
    printf("[Vulkan DEBUG] new window size: %dx%d\n", windowWidth, windowHeight);
    
    cleanupSwapchain();
    createSwapchain();
    createImageViews();
    createFramebuffers();
}

void Vulkan::cleanupSwapchain() {
    printf("[Vulkan DEBUG] cleanupSwapchain\n");
    for (auto fb : framebuffers) vkDestroyFramebuffer(device, fb, nullptr);
    for (auto iv : swapchainImageViews) vkDestroyImageView(device, iv, nullptr);
    if (swapchain) vkDestroySwapchainKHR(device, swapchain, nullptr);
    framebuffers.clear();
    swapchainImageViews.clear();
    swapchainImages.clear();
}

VkShaderModule Vulkan::createShaderModule(const std::vector<char>& code) {
    if (code.empty()) return VK_NULL_HANDLE;
    VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    info.codeSize = code.size();
    info.pCode = reinterpret_cast<const uint32_t*>(code.data());
    VkShaderModule module;
    VkResult result = vkCreateShaderModule(device, &info, nullptr, &module);
    if (result != VK_SUCCESS) {
        printf("[Vulkan ERROR] vkCreateShaderModule failed: %d\n", result);
    }
    return module;
}

uint32_t Vulkan::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physDevice, &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            printf("[Vulkan DEBUG] findMemoryType: found type %u\n", i);
            return i;
        }
    }
    printf("[Vulkan ERROR] findMemoryType: no suitable memory type found!\n");
    return 0;
}