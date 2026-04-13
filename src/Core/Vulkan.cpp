#include "Vulkan.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <cmath>

std::vector<char> Vulkan::readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        printf("[ERROR] Cannot open: %s\n", filename.c_str());
        return {};
    }
    size_t size = file.tellg();
    std::vector<char> buffer(size);
    file.seekg(0);
    file.read(buffer.data(), size);
    file.close();
    printf("[OK] Loaded shader: %s (%zu bytes)\n", filename.c_str(), size);
    return buffer;
}

VkShaderModule Vulkan::createShaderModule(const std::string& filename) {
    auto code = readFile(filename);
    if (code.empty()) return VK_NULL_HANDLE;

    VkShaderModuleCreateInfo info{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    info.codeSize = code.size();
    info.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule module;
    if (vkCreateShaderModule(device, &info, nullptr, &module) != VK_SUCCESS) {
        printf("[ERROR] Failed to create shader module: %s\n", filename.c_str());
        return VK_NULL_HANDLE;
    }
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
    printf("[ERROR] Failed to find suitable memory type!\n");
    return 0;
}

// ==================== CONSTRUCTOR ====================
Vulkan::Vulkan(HWND hwnd, int w, int h)
    : hWnd(hwnd), windowWidth(w), windowHeight(h), rotationAngle(0.0f),
      initialized(false), recording(false) {

    printf("\n=== VULKAN INITIALIZATION START ===\n");

    createInstance();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapchain();
    createImageViews();
    createRenderPass();
    createVertexBuffer2D();
    createCubeResources();
    createPipeline2D();
    createPipeline3D();
    createFramebuffers();
    createCommandPool();
    createSemaphores();

    VkFenceCreateInfo fenceInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    vkCreateFence(device, &fenceInfo, nullptr, &fence);

    initialized = true;
    printf("=== VULKAN INITIALIZED SUCCESSFULLY ===\n\n");
}

Vulkan::~Vulkan() {
    if (!initialized) return;
    vkDeviceWaitIdle(device);
    cleanupSwapchain();

    // 3D cleanup
    if (pipeline3D) vkDestroyPipeline(device, pipeline3D, nullptr);
    if (pipelineLayout3D) vkDestroyPipelineLayout(device, pipelineLayout3D, nullptr);
    if (descriptorSetLayout) vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
    if (descriptorPool) vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    if (uniformBuffer) vkDestroyBuffer(device, uniformBuffer, nullptr);
    if (uniformBufferMemory) vkFreeMemory(device, uniformBufferMemory, nullptr);
    if (vertexBuffer3D) vkDestroyBuffer(device, vertexBuffer3D, nullptr);
    if (vertexBufferMemory3D) vkFreeMemory(device, vertexBufferMemory3D, nullptr);
    if (indexBuffer3D) vkDestroyBuffer(device, indexBuffer3D, nullptr);
    if (indexBufferMemory3D) vkFreeMemory(device, indexBufferMemory3D, nullptr);
    if (vertModule3D) vkDestroyShaderModule(device, vertModule3D, nullptr);
    if (fragModule3D) vkDestroyShaderModule(device, fragModule3D, nullptr);

    // 2D cleanup
    if (fence) vkDestroyFence(device, fence, nullptr);
    if (pipeline2D) vkDestroyPipeline(device, pipeline2D, nullptr);
    if (pipelineLayout2D) vkDestroyPipelineLayout(device, pipelineLayout2D, nullptr);
    if (renderPass) vkDestroyRenderPass(device, renderPass, nullptr);
    if (vertModule2D) vkDestroyShaderModule(device, vertModule2D, nullptr);
    if (fragModule2D) vkDestroyShaderModule(device, fragModule2D, nullptr);
    if (vertexBuffer2D) vkDestroyBuffer(device, vertexBuffer2D, nullptr);
    if (vertexBufferMemory2D) vkFreeMemory(device, vertexBufferMemory2D, nullptr);
    if (commandPool) vkDestroyCommandPool(device, commandPool, nullptr);

    if (imageAvailableSemaphore) vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
    if (renderFinishedSemaphore) vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);

    if (device) vkDestroyDevice(device, nullptr);
    if (surface) vkDestroySurfaceKHR(instance, surface, nullptr);
    if (instance) vkDestroyInstance(instance, nullptr);

    printf("[Vulkan] Cleanup complete\n");
}

// ==================== FRAME MANAGEMENT ====================
void Vulkan::setup2D(int w, int h) {
    windowWidth = w;
    windowHeight = h;
}

void Vulkan::beginFrame() {
    if (!initialized) return;

    vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &fence);

    VkResult res = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX,
        imageAvailableSemaphore, VK_NULL_HANDLE, &currentImageIndex);

    if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
        recreateSwapchain();
        return;
    }

    vkResetCommandBuffer(commandBuffer, 0);
    VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkRenderPassBeginInfo rpBegin{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    rpBegin.renderPass = renderPass;
    rpBegin.framebuffer = framebuffers[currentImageIndex];
    rpBegin.renderArea = {{0, 0}, {uint32_t(windowWidth), uint32_t(windowHeight)}};
    VkClearValue clear = {{0.1f, 0.12f, 0.18f, 1.0f}};
    rpBegin.clearValueCount = 1;
    rpBegin.pClearValues = &clear;

    vkCmdBeginRenderPass(commandBuffer, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);
    recording = true;
}

void Vulkan::endFrame() {
    if (!initialized || !recording) return;
    vkCmdEndRenderPass(commandBuffer);
    vkEndCommandBuffer(commandBuffer);
    recording = false;

    VkSubmitInfo submit{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &imageAvailableSemaphore;
    submit.pWaitDstStageMask = &waitStage;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &commandBuffer;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &renderFinishedSemaphore;

    vkQueueSubmit(graphicsQueue, 1, &submit, fence);
}

void Vulkan::present() {
    if (!initialized) return;

    VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderFinishedSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &currentImageIndex;

    vkQueuePresentKHR(graphicsQueue, &presentInfo);
}

// ==================== DRAW FUNCTIONS ====================
void Vulkan::drawQuad(float x1, float y1, float x2, float y2, float r, float g, float b) {
    if (!initialized || !recording) return;

    float nx1 = (x1 / windowWidth) * 2.0f - 1.0f;
    float ny1 = ((windowHeight - y1) / windowHeight) * 2.0f - 1.0f;
    float nx2 = (x2 / windowWidth) * 2.0f - 1.0f;
    float ny2 = ((windowHeight - y2) / windowHeight) * 2.0f - 1.0f;

    Vertex2D verts[6] = {
        {nx1, ny1, r, g, b, 1.0f},
        {nx2, ny1, r, g, b, 1.0f},
        {nx2, ny2, r, g, b, 1.0f},
        {nx1, ny1, r, g, b, 1.0f},
        {nx2, ny2, r, g, b, 1.0f},
        {nx1, ny2, r, g, b, 1.0f}
    };
    updateVertexBuffer2D(verts, sizeof(verts));

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline2D);
    VkBuffer vb[] = { vertexBuffer2D };
    VkDeviceSize off[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vb, off);
    vkCmdDraw(commandBuffer, 6, 1, 0, 0);
}

void Vulkan::drawQuad(int x1, int y1, int x2, int y2, float r, float g, float b) {
    drawQuad((float)x1, (float)y1, (float)x2, (float)y2, r, g, b);
}

void Vulkan::drawText(int x, int y, const std::string& text, float r, float g, float b) {
    for (size_t i = 0; i < text.size(); ++i) {
        drawQuad(x + i * 10, y, x + (i + 1) * 10, y + 16, r, g, b);
    }
}

void Vulkan::drawTextCentered(int x, int y, int w, int h, const std::string& text, float r, float g, float b) {
    int textWidth = (int)text.size() * 10;
    int startX = x + (w - textWidth) / 2;
    int startY = y + (h - 16) / 2;
    drawText(startX, startY, text, r, g, b);
}

void Vulkan::drawRotatingCube() {
    if (!initialized || !recording) return;

    rotationAngle += 0.02f;
    updateUniformBuffer();

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline3D);
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer3D, offsets);
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer3D, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout3D, 0, 1, &descriptorSet, 0, nullptr);
    vkCmdDrawIndexed(commandBuffer, 36, 1, 0, 0, 0);
}

// ==================== INITIALIZATION FUNCTIONS ====================
void Vulkan::createInstance() {
    VkApplicationInfo appInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
    appInfo.apiVersion = VK_API_VERSION_1_0;

    const char* extensions[] = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME };
    VkInstanceCreateInfo instInfo{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    instInfo.pApplicationInfo = &appInfo;
    instInfo.enabledExtensionCount = 2;
    instInfo.ppEnabledExtensionNames = extensions;

    vkCreateInstance(&instInfo, nullptr, &instance);
}

void Vulkan::createSurface() {
    VkWin32SurfaceCreateInfoKHR win32Info{ VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
    win32Info.hinstance = GetModuleHandle(nullptr);
    win32Info.hwnd = hWnd;
    vkCreateWin32SurfaceKHR(instance, &win32Info, nullptr, &surface);
}

void Vulkan::pickPhysicalDevice() {
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(instance, &count, nullptr);
    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(instance, &count, devices.data());
    physDevice = devices[0];
}

void Vulkan::createLogicalDevice() {
    uint32_t queueFamily = 0;
    VkDeviceQueueCreateInfo queueInfo{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
    queueInfo.queueFamilyIndex = queueFamily;
    queueInfo.queueCount = 1;
    float priority = 1.0f;
    queueInfo.pQueuePriorities = &priority;

    const char* extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    VkDeviceCreateInfo devInfo{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    devInfo.queueCreateInfoCount = 1;
    devInfo.pQueueCreateInfos = &queueInfo;
    devInfo.enabledExtensionCount = 1;
    devInfo.ppEnabledExtensionNames = extensions;

    vkCreateDevice(physDevice, &devInfo, nullptr, &device);
    vkGetDeviceQueue(device, queueFamily, 0, &graphicsQueue);
}

void Vulkan::createSwapchain() {
    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDevice, surface, &caps);

    uint32_t minImageCount = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && minImageCount > caps.maxImageCount)
        minImageCount = caps.maxImageCount;

    VkExtent2D extent = caps.currentExtent;
    if (extent.width == 0 || extent.height == 0) {
        RECT rect;
        GetClientRect(hWnd, &rect);
        extent.width = rect.right - rect.left;
        extent.height = rect.bottom - rect.top;
    }
    if (extent.width == 0) extent.width = 1280;
    if (extent.height == 0) extent.height = 720;

    swapchainExtent = extent;

    VkSurfaceFormatKHR format{ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
    swapchainFormat = format.format;

    VkSwapchainCreateInfoKHR info{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    info.surface = surface;
    info.minImageCount = minImageCount;
    info.imageFormat = format.format;
    info.imageColorSpace = format.colorSpace;
    info.imageExtent = extent;
    info.imageArrayLayers = 1;
    info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    info.clipped = VK_TRUE;

    vkCreateSwapchainKHR(device, &info, nullptr, &swapchain);

    uint32_t imageCount = 0;
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
    swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data());

    printf("[Swapchain] Created with %u images, size %dx%d\n", imageCount, extent.width, extent.height);
}

void Vulkan::createImageViews() {
    swapchainImageViews.resize(swapchainImages.size());
    for (size_t i = 0; i < swapchainImages.size(); ++i) {
        VkImageViewCreateInfo viewInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        viewInfo.image = swapchainImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = swapchainFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;
        vkCreateImageView(device, &viewInfo, nullptr, &swapchainImageViews[i]);
    }
}

void Vulkan::createRenderPass() {
    VkAttachmentDescription colorAtt{};
    colorAtt.format = swapchainFormat;
    colorAtt.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAtt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAtt.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;

    VkRenderPassCreateInfo rpInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    rpInfo.attachmentCount = 1;
    rpInfo.pAttachments = &colorAtt;
    rpInfo.subpassCount = 1;
    rpInfo.pSubpasses = &subpass;

    vkCreateRenderPass(device, &rpInfo, nullptr, &renderPass);
}

void Vulkan::createPipeline2D() {
    vertModule2D = createShaderModule("uiv.spv");
    fragModule2D = createShaderModule("uif.spv");

    if (!vertModule2D || !fragModule2D) {
        printf("[ERROR] Failed to load 2D shaders\n");
        return;
    }

    VkPipelineShaderStageCreateInfo stages[2] = {};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertModule2D;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragModule2D;
    stages[1].pName = "main";

    VkVertexInputBindingDescription bindingDesc{};
    bindingDesc.binding = 0;
    bindingDesc.stride = sizeof(Vertex2D);
    bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attribDesc[2] = {};
    attribDesc[0].binding = 0; attribDesc[0].location = 0; attribDesc[0].format = VK_FORMAT_R32G32_SFLOAT; attribDesc[0].offset = offsetof(Vertex2D, x);
    attribDesc[1].binding = 0; attribDesc[1].location = 1; attribDesc[1].format = VK_FORMAT_R32G32B32A32_SFLOAT; attribDesc[1].offset = offsetof(Vertex2D, r);

    VkPipelineVertexInputStateCreateInfo vertexInput{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &bindingDesc;
    vertexInput.vertexAttributeDescriptionCount = 2;
    vertexInput.pVertexAttributeDescriptions = attribDesc;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkViewport viewport{0, 0, (float)windowWidth, (float)windowHeight, 0, 1};
    VkRect2D scissor{{0,0}, {uint32_t(windowWidth), uint32_t(windowHeight)}};
    VkPipelineViewportStateCreateInfo viewportState{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    viewportState.viewportCount = 1; viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1; viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo raster{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.cullMode = VK_CULL_MODE_NONE;
    raster.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisample{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState blendAtt{};
    blendAtt.blendEnable = VK_TRUE;
    blendAtt.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendAtt.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAtt.colorWriteMask = 0xF;

    VkPipelineColorBlendStateCreateInfo colorBlend{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    colorBlend.attachmentCount = 1;
    colorBlend.pAttachments = &blendAtt;

    VkPipelineLayoutCreateInfo layoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    vkCreatePipelineLayout(device, &layoutInfo, nullptr, &pipelineLayout2D);

    VkGraphicsPipelineCreateInfo pipelineInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = stages;
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &raster;
    pipelineInfo.pMultisampleState = &multisample;
    pipelineInfo.pColorBlendState = &colorBlend;
    pipelineInfo.layout = pipelineLayout2D;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline2D);
    printf("[Pipeline] 2D pipeline created\n");
}

void Vulkan::createPipeline3D() {
    vertModule3D = createShaderModule("3dv.spv");
    fragModule3D = createShaderModule("3df.spv");

    if (!vertModule3D || !fragModule3D) {
        printf("[ERROR] Failed to load 3D shaders\n");
        return;
    }

    VkPipelineShaderStageCreateInfo stages[2] = {};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertModule3D;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragModule3D;
    stages[1].pName = "main";

    VkVertexInputBindingDescription bindingDesc{};
    bindingDesc.binding = 0;
    bindingDesc.stride = sizeof(Vertex3D);
    bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attribDesc[2] = {};
    attribDesc[0].binding = 0; attribDesc[0].location = 0; attribDesc[0].format = VK_FORMAT_R32G32B32_SFLOAT; attribDesc[0].offset = offsetof(Vertex3D, x);
    attribDesc[1].binding = 0; attribDesc[1].location = 1; attribDesc[1].format = VK_FORMAT_R32G32B32A32_SFLOAT; attribDesc[1].offset = offsetof(Vertex3D, r);

    VkPipelineVertexInputStateCreateInfo vertexInput{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &bindingDesc;
    vertexInput.vertexAttributeDescriptionCount = 2;
    vertexInput.pVertexAttributeDescriptions = attribDesc;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkViewport viewport{0, 0, (float)windowWidth, (float)windowHeight, 0, 1};
    VkRect2D scissor{{0,0}, {uint32_t(windowWidth), uint32_t(windowHeight)}};
    VkPipelineViewportStateCreateInfo viewportState{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    viewportState.viewportCount = 1; viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1; viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo raster{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.cullMode = VK_CULL_MODE_BACK_BIT;
    raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisample{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState blendAtt{};
    blendAtt.colorWriteMask = 0xF;

    VkPipelineColorBlendStateCreateInfo colorBlend{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    colorBlend.attachmentCount = 1;
    colorBlend.pAttachments = &blendAtt;

    VkPipelineDepthStencilStateCreateInfo depthStencil{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

    VkPipelineLayoutCreateInfo layoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &descriptorSetLayout;

    vkCreatePipelineLayout(device, &layoutInfo, nullptr, &pipelineLayout3D);

    VkGraphicsPipelineCreateInfo pipelineInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = stages;
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &raster;
    pipelineInfo.pMultisampleState = &multisample;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlend;
    pipelineInfo.layout = pipelineLayout3D;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline3D);
    printf("[Pipeline] 3D pipeline created\n");
}

void Vulkan::createFramebuffers() {
    framebuffers.resize(swapchainImageViews.size());
    for (size_t i = 0; i < swapchainImageViews.size(); ++i) {
        VkFramebufferCreateInfo fbInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        fbInfo.renderPass = renderPass;
        fbInfo.attachmentCount = 1;
        fbInfo.pAttachments = &swapchainImageViews[i];
        fbInfo.width = uint32_t(windowWidth);
        fbInfo.height = uint32_t(windowHeight);
        fbInfo.layers = 1;
        vkCreateFramebuffer(device, &fbInfo, nullptr, &framebuffers[i]);
    }
}

void Vulkan::createCommandPool() {
    VkCommandPoolCreateInfo poolInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool);

    VkCommandBufferAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
}

void Vulkan::createSemaphores() {
    VkSemaphoreCreateInfo semInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    vkCreateSemaphore(device, &semInfo, nullptr, &imageAvailableSemaphore);
    vkCreateSemaphore(device, &semInfo, nullptr, &renderFinishedSemaphore);
}

void Vulkan::createVertexBuffer2D() {
    VkBufferCreateInfo bufferInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufferInfo.size = 1024 * 1024;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    vkCreateBuffer(device, &bufferInfo, nullptr, &vertexBuffer2D);

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(device, vertexBuffer2D, &memReqs);
    VkMemoryAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkAllocateMemory(device, &allocInfo, nullptr, &vertexBufferMemory2D);
    vkBindBufferMemory(device, vertexBuffer2D, vertexBufferMemory2D, 0);
}

void Vulkan::updateVertexBuffer2D(const void* data, size_t size) {
    void* mapped;
    vkMapMemory(device, vertexBufferMemory2D, 0, size, 0, &mapped);
    memcpy(mapped, data, size);
    vkUnmapMemory(device, vertexBufferMemory2D);
}

void Vulkan::createCubeResources() {
    printf("[Cube] Creating cube resources\n");

    Vertex3D vertices[] = {
        {-0.5f,-0.5f, 0.5f, 1,0,0,1}, { 0.5f,-0.5f, 0.5f, 1,0,0,1}, { 0.5f, 0.5f, 0.5f, 1,0,0,1}, {-0.5f, 0.5f, 0.5f, 1,0,0,1},
        {-0.5f,-0.5f,-0.5f, 0,1,0,1}, { 0.5f,-0.5f,-0.5f, 0,1,0,1}, { 0.5f, 0.5f,-0.5f, 0,1,0,1}, {-0.5f, 0.5f,-0.5f, 0,1,0,1},
        {-0.5f, 0.5f,-0.5f, 0,0,1,1}, { 0.5f, 0.5f,-0.5f, 0,0,1,1}, { 0.5f, 0.5f, 0.5f, 0,0,1,1}, {-0.5f, 0.5f, 0.5f, 0,0,1,1},
        {-0.5f,-0.5f,-0.5f, 1,1,0,1}, { 0.5f,-0.5f,-0.5f, 1,1,0,1}, { 0.5f,-0.5f, 0.5f, 1,1,0,1}, {-0.5f,-0.5f, 0.5f, 1,1,0,1},
        { 0.5f,-0.5f,-0.5f, 1,0,1,1}, { 0.5f, 0.5f,-0.5f, 1,0,1,1}, { 0.5f, 0.5f, 0.5f, 1,0,1,1}, { 0.5f,-0.5f, 0.5f, 1,0,1,1},
        {-0.5f,-0.5f,-0.5f, 0,1,1,1}, {-0.5f, 0.5f,-0.5f, 0,1,1,1}, {-0.5f, 0.5f, 0.5f, 0,1,1,1}, {-0.5f,-0.5f, 0.5f, 0,1,1,1}
    };

    uint32_t indices[] = {
        0,1,2, 0,2,3,
        4,5,6, 4,6,7,
        8,9,10,8,10,11,
        12,13,14,12,14,15,
        16,17,18,16,18,19,
        20,21,22,20,22,23
    };

    // Vertex Buffer
    VkBufferCreateInfo bufferInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufferInfo.size = sizeof(vertices);
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    vkCreateBuffer(device, &bufferInfo, nullptr, &vertexBuffer3D);

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(device, vertexBuffer3D, &memReqs);
    VkMemoryAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkAllocateMemory(device, &allocInfo, nullptr, &vertexBufferMemory3D);
    vkBindBufferMemory(device, vertexBuffer3D, vertexBufferMemory3D, 0);

    void* data;
    vkMapMemory(device, vertexBufferMemory3D, 0, sizeof(vertices), 0, &data);
    memcpy(data, vertices, sizeof(vertices));
    vkUnmapMemory(device, vertexBufferMemory3D);

    // Index Buffer
    bufferInfo.size = sizeof(indices);
    bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    vkCreateBuffer(device, &bufferInfo, nullptr, &indexBuffer3D);
    vkGetBufferMemoryRequirements(device, indexBuffer3D, &memReqs);
    allocInfo.allocationSize = memReqs.size;
    vkAllocateMemory(device, &allocInfo, nullptr, &indexBufferMemory3D);
    vkBindBufferMemory(device, indexBuffer3D, indexBufferMemory3D, 0);

    vkMapMemory(device, indexBufferMemory3D, 0, sizeof(indices), 0, &data);
    memcpy(data, indices, sizeof(indices));
    vkUnmapMemory(device, indexBufferMemory3D);

    createUniformBuffer();
    createDescriptorSet();
}

void Vulkan::createUniformBuffer() {
    VkBufferCreateInfo bufferInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufferInfo.size = sizeof(float) * 16 * 3;
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    vkCreateBuffer(device, &bufferInfo, nullptr, &uniformBuffer);

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(device, uniformBuffer, &memReqs);
    VkMemoryAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkAllocateMemory(device, &allocInfo, nullptr, &uniformBufferMemory);
    vkBindBufferMemory(device, uniformBuffer, uniformBufferMemory, 0);
    vkMapMemory(device, uniformBufferMemory, 0, bufferInfo.size, 0, &uniformBufferMapped);
}

void Vulkan::createDescriptorSet() {
    VkDescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding = 0;
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &layoutBinding;
    vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout);

    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool);

    VkDescriptorSetAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout;
    vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet);

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = uniformBuffer;
    bufferInfo.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet writeSet{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    writeSet.dstSet = descriptorSet;
    writeSet.dstBinding = 0;
    writeSet.descriptorCount = 1;
    writeSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeSet.pBufferInfo = &bufferInfo;
    vkUpdateDescriptorSets(device, 1, &writeSet, 0, nullptr);
}

void Vulkan::updateUniformBuffer() {
    struct Mat4 { float m[16]; };
    struct UniformBuffer { Mat4 model; Mat4 view; Mat4 proj; } ubo;

    float angle = rotationAngle;
    float cosY = cosf(angle), sinY = sinf(angle);
    float cosX = cosf(angle * 0.5f), sinX = sinf(angle * 0.5f);

    memset(&ubo.model, 0, sizeof(Mat4));
    ubo.model.m[0] = cosY; ubo.model.m[2] = -sinY;
    ubo.model.m[5] = cosX; ubo.model.m[6] = sinX;
    ubo.model.m[8] = sinY; ubo.model.m[10] = cosY * cosX;
    ubo.model.m[15] = 1.0f;

    memset(&ubo.view, 0, sizeof(Mat4));
    ubo.view.m[0] = 1.0f; ubo.view.m[5] = 1.0f; ubo.view.m[10] = 1.0f;
    ubo.view.m[14] = -3.0f; ubo.view.m[15] = 1.0f;

    float aspect = (float)windowWidth / (float)windowHeight;
    float fov = 3.14159f / 3.0f;
    float zNear = 0.1f, zFar = 10.0f;

    memset(&ubo.proj, 0, sizeof(Mat4));
    float tanHalfFov = tanf(fov / 2.0f);
    ubo.proj.m[0] = 1.0f / (aspect * tanHalfFov);
    ubo.proj.m[5] = 1.0f / tanHalfFov;
    ubo.proj.m[10] = zFar / (zFar - zNear);
    ubo.proj.m[11] = 1.0f;
    ubo.proj.m[14] = -(zFar * zNear) / (zFar - zNear);

    memcpy(uniformBufferMapped, &ubo, sizeof(ubo));
}

void Vulkan::recreateSwapchain() {
    vkDeviceWaitIdle(device);
    cleanupSwapchain();
    createSwapchain();
    createImageViews();
    createFramebuffers();
}

void Vulkan::cleanupSwapchain() {
    for (auto fb : framebuffers) vkDestroyFramebuffer(device, fb, nullptr);
    for (auto iv : swapchainImageViews) vkDestroyImageView(device, iv, nullptr);
    if (swapchain) vkDestroySwapchainKHR(device, swapchain, nullptr);
    framebuffers.clear();
    swapchainImageViews.clear();
    swapchainImages.clear();
}