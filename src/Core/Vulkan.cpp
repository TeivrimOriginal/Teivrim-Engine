#include "Vulkan.h"
#include <iostream>
#include <fstream>
#include <cstring>

std::vector<char> Vulkan::readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        printf("[Vulkan] Failed to open: %s\n", filename.c_str());
        return {};
    }
    size_t size = file.tellg();
    std::vector<char> buffer(size);
    file.seekg(0);
    file.read(buffer.data(), size);
    file.close();
    return buffer;
}

Vulkan::Vulkan(HWND hwnd, int w, int h) 
    : hWnd(hwnd), width(w), height(h), currentImageIndex(0), initialized(false), 
      inRenderPass(false), vertexCount3D(0), vertexSize3D(0),
      instance(VK_NULL_HANDLE), physDevice(VK_NULL_HANDLE), device(VK_NULL_HANDLE),
      surface(VK_NULL_HANDLE), swapchain(VK_NULL_HANDLE), commandPool(VK_NULL_HANDLE),
      commandBuffer(VK_NULL_HANDLE), imageAvailableSemaphore(VK_NULL_HANDLE),
      renderFinishedSemaphore(VK_NULL_HANDLE),
      renderPass3D(VK_NULL_HANDLE), pipelineLayout3D(VK_NULL_HANDLE), pipeline3D(VK_NULL_HANDLE),
      vertexBuffer3D(VK_NULL_HANDLE), vertexBufferMemory3D(VK_NULL_HANDLE),
      vertModule3D(VK_NULL_HANDLE), fragModule3D(VK_NULL_HANDLE),
      renderPassUI(VK_NULL_HANDLE), pipelineLayoutUI(VK_NULL_HANDLE), pipelineUI(VK_NULL_HANDLE),
      vertexBufferUI(VK_NULL_HANDLE), vertexBufferMemoryUI(VK_NULL_HANDLE),
      vertModuleUI(VK_NULL_HANDLE), fragModuleUI(VK_NULL_HANDLE) {
    
    printf("[Vulkan] Initializing...\n");
    
    createInstance();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapchain();
    createImageViews();
    createCommandPool();
    createSemaphores();
    createVertexBuffers();
    
    createRenderPass3D();
    createPipeline3D();
    createFramebuffers3D();
    
    createRenderPassUI();
    createPipelineUI();
    createFramebuffersUI();
    
    initialized = true;
    printf("[Vulkan] Initialized successfully!\n");
}

Vulkan::~Vulkan() {
    if (!initialized) return;
    vkDeviceWaitIdle(device);
    
    cleanupSwapchain();
    
    vkDestroyPipeline(device, pipelineUI, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayoutUI, nullptr);
    vkDestroyRenderPass(device, renderPassUI, nullptr);
    
    vkDestroyPipeline(device, pipeline3D, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout3D, nullptr);
    vkDestroyRenderPass(device, renderPass3D, nullptr);
    
    vkDestroyShaderModule(device, vertModuleUI, nullptr);
    vkDestroyShaderModule(device, fragModuleUI, nullptr);
    vkDestroyShaderModule(device, vertModule3D, nullptr);
    vkDestroyShaderModule(device, fragModule3D, nullptr);
    
    vkDestroyBuffer(device, vertexBufferUI, nullptr);
    vkFreeMemory(device, vertexBufferMemoryUI, nullptr);
    vkDestroyBuffer(device, vertexBuffer3D, nullptr);
    vkFreeMemory(device, vertexBufferMemory3D, nullptr);
    
    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
    vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
    
    printf("[Vulkan] Cleanup complete\n");
}

void Vulkan::setup2D(int w, int h) {
    width = w;
    height = h;
}

void Vulkan::beginFrame() {
    vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &currentImageIndex);
    vkResetCommandBuffer(commandBuffer, 0);
    
    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    
    // Начинаем 3D render pass (очищает экран)
    VkRenderPassBeginInfo rpBegin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    rpBegin.renderPass = renderPass3D;
    rpBegin.framebuffer = framebuffers3D[currentImageIndex];
    rpBegin.renderArea.extent = {uint32_t(width), uint32_t(height)};
    VkClearValue clearColor = {0.1f, 0.1f, 0.2f, 1.0f};
    rpBegin.clearValueCount = 1;
    rpBegin.pClearValues = &clearColor;
    
    vkCmdBeginRenderPass(commandBuffer, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);
    inRenderPass = true;
}

void Vulkan::endFrame() {
    // Заканчиваем 3D render pass
    if (inRenderPass) {
        vkCmdEndRenderPass(commandBuffer);
        inRenderPass = false;
    }
    
    vkEndCommandBuffer(commandBuffer);
    
    VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &imageAvailableSemaphore;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderFinishedSemaphore;
    
    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
}

void Vulkan::present() {
    VkPresentInfoKHR presentInfo{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderFinishedSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &currentImageIndex;
    
    vkQueuePresentKHR(graphicsQueue, &presentInfo);
    vkQueueWaitIdle(graphicsQueue);
}

void Vulkan::drawQuad(float x1, float y1, float x2, float y2, float r, float g, float b) {
    if (!initialized) return;
    
    // Заканчиваем 3D pass если он активен
    if (inRenderPass) {
        vkCmdEndRenderPass(commandBuffer);
        inRenderPass = false;
    }
    
    // Начинаем UI render pass (если ещё не начат)
    static bool uiPassStarted = false;
    if (!uiPassStarted) {
        VkRenderPassBeginInfo rpBegin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        rpBegin.renderPass = renderPassUI;
        rpBegin.framebuffer = framebuffersUI[currentImageIndex];
        rpBegin.renderArea.extent = {uint32_t(width), uint32_t(height)};
        rpBegin.clearValueCount = 0;
        
        vkCmdBeginRenderPass(commandBuffer, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);
        uiPassStarted = true;
    }
    
    float nx1 = (x1 / width) * 2.0f - 1.0f;
    float ny1 = ((height - y1) / height) * 2.0f - 1.0f;
    float nx2 = (x2 / width) * 2.0f - 1.0f;
    float ny2 = ((height - y2) / height) * 2.0f - 1.0f;
    
    std::vector<Vertex2D> vertices = {
        {nx1, ny1, r, g, b, 1.0f},
        {nx2, ny1, r, g, b, 1.0f},
        {nx2, ny2, r, g, b, 1.0f},
        {nx1, ny1, r, g, b, 1.0f},
        {nx2, ny2, r, g, b, 1.0f},
        {nx1, ny2, r, g, b, 1.0f}
    };
    
    updateUIBuffer(vertices);
    
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineUI);
    VkBuffer vertexBuffers[] = {vertexBufferUI};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdDraw(commandBuffer, 6, 1, 0, 0);
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
    int textWidth = text.size() * 10;
    int startX = x + (w - textWidth) / 2;
    int startY = y + (h - 16) / 2;
    drawText(startX, startY, text, r, g, b);
}

void Vulkan::drawTriangle() {
    if (!initialized) return;
    
    std::vector<Vertex3D> vertices = {
        {0.0f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f},
        {0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f},
        {-0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f}
    };
    
    update3DBuffer(vertices);
    vertexCount3D = 3;
    
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline3D);
    VkBuffer vertexBuffers[] = {vertexBuffer3D};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdDraw(commandBuffer, vertexCount3D, 1, 0, 0);
}

void Vulkan::set3DData(void* data, int count, int size) {
    if (data && count > 0) {
        std::vector<Vertex3D> vertices((Vertex3D*)data, (Vertex3D*)data + count);
        update3DBuffer(vertices);
        vertexCount3D = count;
        vertexSize3D = size;
    }
}

// ==================== PRIVATE METHODS ====================

void Vulkan::createInstance() {
    VkApplicationInfo appInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    appInfo.apiVersion = VK_API_VERSION_1_0;
    
    const char* extensions[] = {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME};
    VkInstanceCreateInfo instInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instInfo.pApplicationInfo = &appInfo;
    instInfo.enabledExtensionCount = 2;
    instInfo.ppEnabledExtensionNames = extensions;
    vkCreateInstance(&instInfo, nullptr, &instance);
}

void Vulkan::createSurface() {
    VkWin32SurfaceCreateInfoKHR win32Info{VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
    win32Info.hinstance = GetModuleHandle(nullptr);
    win32Info.hwnd = hWnd;
    vkCreateWin32SurfaceKHR(instance, &win32Info, nullptr, &surface);
}

void Vulkan::pickPhysicalDevice() {
    uint32_t deviceCount;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
    physDevice = devices[0];
    
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(physDevice, &props);
    printf("[Vulkan] GPU: %s\n", props.deviceName);
}

void Vulkan::createLogicalDevice() {
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
    vkCreateDevice(physDevice, &devInfo, nullptr, &device);
    vkGetDeviceQueue(device, queueFamily, 0, &graphicsQueue);
}

void Vulkan::createSwapchain() {
    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDevice, surface, &caps);
    
    VkSurfaceFormatKHR format{VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    swapchainFormat = format.format;
    swapchainExtent = caps.currentExtent;
    if (swapchainExtent.width == 0) swapchainExtent = {uint32_t(width), uint32_t(height)};
    
    VkSwapchainCreateInfoKHR swapInfo{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    swapInfo.surface = surface;
    swapInfo.minImageCount = caps.minImageCount + 1;
    swapInfo.imageFormat = format.format;
    swapInfo.imageColorSpace = format.colorSpace;
    swapInfo.imageExtent = swapchainExtent;
    swapInfo.imageArrayLayers = 1;
    swapInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapInfo.preTransform = caps.currentTransform;
    swapInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapInfo.clipped = VK_TRUE;
    vkCreateSwapchainKHR(device, &swapInfo, nullptr, &swapchain);
    
    uint32_t imageCount;
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
    swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data());
}

void Vulkan::createImageViews() {
    swapchainImageViews.resize(swapchainImages.size());
    for (size_t i = 0; i < swapchainImages.size(); i++) {
        VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        viewInfo.image = swapchainImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = swapchainFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;
        vkCreateImageView(device, &viewInfo, nullptr, &swapchainImageViews[i]);
    }
}

void Vulkan::createCommandPool() {
    VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool);
    
    VkCommandBufferAllocateInfo allocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
}

void Vulkan::createSemaphores() {
    VkSemaphoreCreateInfo semInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    vkCreateSemaphore(device, &semInfo, nullptr, &imageAvailableSemaphore);
    vkCreateSemaphore(device, &semInfo, nullptr, &renderFinishedSemaphore);
}

void Vulkan::createVertexBuffers() {
    VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = sizeof(Vertex2D) * 6;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkCreateBuffer(device, &bufferInfo, nullptr, &vertexBufferUI);
    
    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(device, vertexBufferUI, &memReqs);
    VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, 
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkAllocateMemory(device, &allocInfo, nullptr, &vertexBufferMemoryUI);
    vkBindBufferMemory(device, vertexBufferUI, vertexBufferMemoryUI, 0);
    
    bufferInfo.size = sizeof(Vertex3D) * 1024; // Достаточно для моделей
    vkCreateBuffer(device, &bufferInfo, nullptr, &vertexBuffer3D);
    vkGetBufferMemoryRequirements(device, vertexBuffer3D, &memReqs);
    vkAllocateMemory(device, &allocInfo, nullptr, &vertexBufferMemory3D);
    vkBindBufferMemory(device, vertexBuffer3D, vertexBufferMemory3D, 0);
}

void Vulkan::createRenderPass3D() {
    VkAttachmentDescription colorAtt{};
    colorAtt.format = swapchainFormat;
    colorAtt.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAtt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAtt.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
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
    vkCreateRenderPass(device, &rpInfo, nullptr, &renderPass3D);
}

void Vulkan::createPipeline3D() {
    vertModule3D = createShaderModule("vert.spv");
    fragModule3D = createShaderModule("frag.spv");
    
    VkPipelineShaderStageCreateInfo stages[2] = {};
    stages[0] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertModule3D;
    stages[0].pName = "main";
    stages[1] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragModule3D;
    stages[1].pName = "main";
    
    VkVertexInputBindingDescription bindingDesc{};
    bindingDesc.binding = 0;
    bindingDesc.stride = sizeof(Vertex3D);
    bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    VkVertexInputAttributeDescription attribDesc[2]{};
    attribDesc[0].binding = 0;
    attribDesc[0].location = 0;
    attribDesc[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attribDesc[0].offset = offsetof(Vertex3D, x);
    attribDesc[1].binding = 0;
    attribDesc[1].location = 1;
    attribDesc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attribDesc[1].offset = offsetof(Vertex3D, r);
    
    VkPipelineVertexInputStateCreateInfo vertexInput{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &bindingDesc;
    vertexInput.vertexAttributeDescriptionCount = 2;
    vertexInput.pVertexAttributeDescriptions = attribDesc;
    
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    
    VkViewport viewport{0, 0, (float)width, (float)height, 0, 1};
    VkRect2D scissor{{0,0}, {uint32_t(width), uint32_t(height)}};
    VkPipelineViewportStateCreateInfo viewportState{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;
    
    VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.cullMode = VK_CULL_MODE_BACK_BIT;
    raster.lineWidth = 1.0f;
    
    VkPipelineMultisampleStateCreateInfo multisample{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    
    VkPipelineColorBlendAttachmentState blendAtt{};
    blendAtt.colorWriteMask = 0xF;
    VkPipelineColorBlendStateCreateInfo colorBlend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    colorBlend.attachmentCount = 1;
    colorBlend.pAttachments = &blendAtt;
    
    VkPipelineLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    vkCreatePipelineLayout(device, &layoutInfo, nullptr, &pipelineLayout3D);
    
    VkGraphicsPipelineCreateInfo pipelineInfo{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = stages;
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &raster;
    pipelineInfo.pMultisampleState = &multisample;
    pipelineInfo.pColorBlendState = &colorBlend;
    pipelineInfo.layout = pipelineLayout3D;
    pipelineInfo.renderPass = renderPass3D;
    pipelineInfo.subpass = 0;
    
    vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline3D);
}

void Vulkan::createFramebuffers3D() {
    framebuffers3D.resize(swapchainImageViews.size());
    for (size_t i = 0; i < swapchainImageViews.size(); i++) {
        VkFramebufferCreateInfo fbInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        fbInfo.renderPass = renderPass3D;
        fbInfo.attachmentCount = 1;
        fbInfo.pAttachments = &swapchainImageViews[i];
        fbInfo.width = swapchainExtent.width;
        fbInfo.height = swapchainExtent.height;
        fbInfo.layers = 1;
        vkCreateFramebuffer(device, &fbInfo, nullptr, &framebuffers3D[i]);
    }
}

void Vulkan::createRenderPassUI() {
    VkAttachmentDescription colorAtt{};
    colorAtt.format = swapchainFormat;
    colorAtt.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAtt.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    colorAtt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAtt.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
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
    vkCreateRenderPass(device, &rpInfo, nullptr, &renderPassUI);
}

void Vulkan::createPipelineUI() {
    vertModuleUI = createShaderModule("ui_vert.spv");
    fragModuleUI = createShaderModule("ui_frag.spv");
    
    VkPipelineShaderStageCreateInfo stages[2] = {};
    stages[0] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertModuleUI;
    stages[0].pName = "main";
    stages[1] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragModuleUI;
    stages[1].pName = "main";
    
    VkVertexInputBindingDescription bindingDesc{};
    bindingDesc.binding = 0;
    bindingDesc.stride = sizeof(Vertex2D);
    bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    VkVertexInputAttributeDescription attribDesc[2]{};
    attribDesc[0].binding = 0;
    attribDesc[0].location = 0;
    attribDesc[0].format = VK_FORMAT_R32G32_SFLOAT;
    attribDesc[0].offset = offsetof(Vertex2D, x);
    attribDesc[1].binding = 0;
    attribDesc[1].location = 1;
    attribDesc[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attribDesc[1].offset = offsetof(Vertex2D, r);
    
    VkPipelineVertexInputStateCreateInfo vertexInput{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &bindingDesc;
    vertexInput.vertexAttributeDescriptionCount = 2;
    vertexInput.pVertexAttributeDescriptions = attribDesc;
    
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    
    VkViewport viewport{0, 0, (float)width, (float)height, 0, 1};
    VkRect2D scissor{{0,0}, {uint32_t(width), uint32_t(height)}};
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
    vkCreatePipelineLayout(device, &layoutInfo, nullptr, &pipelineLayoutUI);
    
    VkGraphicsPipelineCreateInfo pipelineInfo{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = stages;
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &raster;
    pipelineInfo.pMultisampleState = &multisample;
    pipelineInfo.pColorBlendState = &colorBlend;
    pipelineInfo.layout = pipelineLayoutUI;
    pipelineInfo.renderPass = renderPassUI;
    pipelineInfo.subpass = 0;
    
    vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipelineUI);
}

void Vulkan::createFramebuffersUI() {
    framebuffersUI.resize(swapchainImageViews.size());
    for (size_t i = 0; i < swapchainImageViews.size(); i++) {
        VkFramebufferCreateInfo fbInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        fbInfo.renderPass = renderPassUI;
        fbInfo.attachmentCount = 1;
        fbInfo.pAttachments = &swapchainImageViews[i];
        fbInfo.width = swapchainExtent.width;
        fbInfo.height = swapchainExtent.height;
        fbInfo.layers = 1;
        vkCreateFramebuffer(device, &fbInfo, nullptr, &framebuffersUI[i]);
    }
}

void Vulkan::recreateSwapchain() {
    vkDeviceWaitIdle(device);
    cleanupSwapchain();
    createSwapchain();
    createImageViews();
    createFramebuffers3D();
    createFramebuffersUI();
}

void Vulkan::cleanupSwapchain() {
    for (auto fb : framebuffers3D) vkDestroyFramebuffer(device, fb, nullptr);
    for (auto fb : framebuffersUI) vkDestroyFramebuffer(device, fb, nullptr);
    for (auto iv : swapchainImageViews) vkDestroyImageView(device, iv, nullptr);
    if (swapchain) vkDestroySwapchainKHR(device, swapchain, nullptr);
    framebuffers3D.clear();
    framebuffersUI.clear();
    swapchainImageViews.clear();
    swapchainImages.clear();
}

void Vulkan::updateUIBuffer(const std::vector<Vertex2D>& vertices) {
    void* data;
    vkMapMemory(device, vertexBufferMemoryUI, 0, sizeof(Vertex2D) * vertices.size(), 0, &data);
    memcpy(data, vertices.data(), sizeof(Vertex2D) * vertices.size());
    vkUnmapMemory(device, vertexBufferMemoryUI);
}

void Vulkan::update3DBuffer(const std::vector<Vertex3D>& vertices) {
    void* data;
    vkMapMemory(device, vertexBufferMemory3D, 0, sizeof(Vertex3D) * vertices.size(), 0, &data);
    memcpy(data, vertices.data(), sizeof(Vertex3D) * vertices.size());
    vkUnmapMemory(device, vertexBufferMemory3D);
}

VkShaderModule Vulkan::createShaderModule(const std::string& filename) {
    auto code = readFile(filename);
    if (code.empty()) {
        printf("[Vulkan] Shader not found: %s\n", filename.c_str());
        return VK_NULL_HANDLE;
    }
    VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    info.codeSize = code.size();
    info.pCode = reinterpret_cast<const uint32_t*>(code.data());
    VkShaderModule module;
    vkCreateShaderModule(device, &info, nullptr, &module);
    return module;
}

uint32_t Vulkan::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physDevice, &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    return 0;
}