#include "VkInit.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>

VkContext VkInit::ctx;
bool VkInit::initialized = false;
uint32_t VkInit::currentImageIndex = 0;

VkContext& VkInit::getContext() { return ctx; }

std::vector<char> VkInit::readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        printf("[Vulkan] Failed to open: %s\n", filename.c_str());
        return {};
    }
    size_t size = file.tellg();
    file.seekg(0);
    std::vector<char> buffer(size);
    file.read(buffer.data(), size);
    file.close();
    return buffer;
}

VkShaderModule VkInit::createShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    info.codeSize = code.size();
    info.pCode = reinterpret_cast<const uint32_t*>(code.data());
    VkShaderModule module;
    if (vkCreateShaderModule(ctx.device, &info, nullptr, &module) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }
    return module;
}

uint32_t VkInit::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(ctx.physicalDevice, &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }
    return 0;
}

bool VkInit::createInstance() {
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.apiVersion = VK_API_VERSION_1_0;
    
    const char* extensions[] = {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME};
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = 2;
    createInfo.ppEnabledExtensionNames = extensions;
    
    return vkCreateInstance(&createInfo, nullptr, &ctx.instance) == VK_SUCCESS;
}

bool VkInit::createSurface() {
    VkWin32SurfaceCreateInfoKHR surfaceInfo{};
    surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceInfo.hinstance = GetModuleHandle(nullptr);
    surfaceInfo.hwnd = ctx.hwnd;
    
    auto func = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(ctx.instance, "vkCreateWin32SurfaceKHR");
    if (!func) return false;
    return func(ctx.instance, &surfaceInfo, nullptr, &ctx.surface) == VK_SUCCESS;
}

bool VkInit::pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(ctx.instance, &deviceCount, nullptr);
    if (deviceCount == 0) return false;
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(ctx.instance, &deviceCount, devices.data());
    ctx.physicalDevice = devices[0];
    
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(ctx.physicalDevice, &props);
    printf("[Vulkan] GPU: %s\n", props.deviceName);
    return true;
}

bool VkInit::createLogicalDevice() {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(ctx.physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(ctx.physicalDevice, &queueFamilyCount, queueFamilies.data());
    
    uint32_t graphicsFamily = 0;
    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphicsFamily = i;
            break;
        }
    }
    
    float priority = 1.0f;
    VkDeviceQueueCreateInfo queueInfo{};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = graphicsFamily;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &priority;
    
    const char* extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pQueueCreateInfos = &queueInfo;
    createInfo.enabledExtensionCount = 1;
    createInfo.ppEnabledExtensionNames = extensions;
    
    if (vkCreateDevice(ctx.physicalDevice, &createInfo, nullptr, &ctx.device) != VK_SUCCESS) return false;
    vkGetDeviceQueue(ctx.device, graphicsFamily, 0, &ctx.graphicsQueue);
    return true;
}

bool VkInit::createSwapChain() {
    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx.physicalDevice, ctx.surface, &caps);
    
    uint32_t imageCount = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount) imageCount = caps.maxImageCount;
    
    ctx.swapChainExtent = caps.currentExtent;
    if (ctx.swapChainExtent.width == 0xFFFFFFFF) {
        ctx.swapChainExtent.width = ctx.width;
        ctx.swapChainExtent.height = ctx.height;
    }
    
    VkSurfaceFormatKHR format{VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    ctx.swapChainImageFormat = format.format;
    
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = ctx.surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = format.format;
    createInfo.imageColorSpace = format.colorSpace;
    createInfo.imageExtent = ctx.swapChainExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.preTransform = caps.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    createInfo.clipped = VK_TRUE;
    
    if (vkCreateSwapchainKHR(ctx.device, &createInfo, nullptr, &ctx.swapChain) != VK_SUCCESS) return false;
    
    vkGetSwapchainImagesKHR(ctx.device, ctx.swapChain, &imageCount, nullptr);
    ctx.swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(ctx.device, ctx.swapChain, &imageCount, ctx.swapChainImages.data());
    return true;
}

bool VkInit::createImageViews() {
    ctx.swapChainImageViews.resize(ctx.swapChainImages.size());
    for (size_t i = 0; i < ctx.swapChainImages.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = ctx.swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = ctx.swapChainImageFormat;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.layerCount = 1;
        if (vkCreateImageView(ctx.device, &createInfo, nullptr, &ctx.swapChainImageViews[i]) != VK_SUCCESS)
            return false;
    }
    return true;
}

bool VkInit::createCommandPool() {
    VkCommandPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    if (vkCreateCommandPool(ctx.device, &createInfo, nullptr, &ctx.commandPool) != VK_SUCCESS) return false;
    
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = ctx.commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    return vkAllocateCommandBuffers(ctx.device, &allocInfo, &ctx.commandBuffer) == VK_SUCCESS;
}

bool VkInit::createSemaphores() {
    VkSemaphoreCreateInfo semInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    if (vkCreateSemaphore(ctx.device, &semInfo, nullptr, &ctx.imageAvailableSemaphore) != VK_SUCCESS) return false;
    if (vkCreateSemaphore(ctx.device, &semInfo, nullptr, &ctx.renderFinishedSemaphore) != VK_SUCCESS) return false;
    return true;
}

bool VkInit::createVertexBuffers() {
    VkDeviceSize bufferSize = sizeof(float) * 6 * 65536;
    
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateBuffer(ctx.device, &bufferInfo, nullptr, &ctx.vertexBuffer3D) != VK_SUCCESS) return false;
    if (vkCreateBuffer(ctx.device, &bufferInfo, nullptr, &ctx.vertexBufferUI) != VK_SUCCESS) return false;
    
    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(ctx.device, ctx.vertexBuffer3D, &memReqs);
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, 
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    if (vkAllocateMemory(ctx.device, &allocInfo, nullptr, &ctx.vertexBufferMemory3D) != VK_SUCCESS) return false;
    if (vkAllocateMemory(ctx.device, &allocInfo, nullptr, &ctx.vertexBufferMemoryUI) != VK_SUCCESS) return false;
    
    vkBindBufferMemory(ctx.device, ctx.vertexBuffer3D, ctx.vertexBufferMemory3D, 0);
    vkBindBufferMemory(ctx.device, ctx.vertexBufferUI, ctx.vertexBufferMemoryUI, 0);
    return true;
}

bool VkInit::createRenderPass3D() {
    VkAttachmentDescription colorAtt{};
    colorAtt.format = ctx.swapChainImageFormat;
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
    
    return vkCreateRenderPass(ctx.device, &rpInfo, nullptr, &ctx.renderPass3D) == VK_SUCCESS;
}

bool VkInit::createPipeline3D() {
    auto vertCode = readFile("vert.spv");
    auto fragCode = readFile("frag.spv");
    if (vertCode.empty() || fragCode.empty()) {
        printf("[Vulkan] Failed to load 3D shaders\n");
        return false;
    }
    
    VkShaderModule vertModule = createShaderModule(vertCode);
    VkShaderModule fragModule = createShaderModule(fragCode);
    if (!vertModule || !fragModule) return false;
    
    VkPipelineShaderStageCreateInfo stages[2] = {};
    stages[0] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertModule;
    stages[0].pName = "main";
    stages[1] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragModule;
    stages[1].pName = "main";
    
    VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = sizeof(float) * 6;
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    VkVertexInputAttributeDescription attributes[2];
    attributes[0] = {0, 0, VK_FORMAT_R32G32_SFLOAT, 0};
    attributes[1] = {1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, sizeof(float) * 2};
    
    VkPipelineVertexInputStateCreateInfo vertexInput{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &binding;
    vertexInput.vertexAttributeDescriptionCount = 2;
    vertexInput.pVertexAttributeDescriptions = attributes;
    
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    
    VkViewport viewport{0, 0, (float)ctx.swapChainExtent.width, (float)ctx.swapChainExtent.height, 0, 1};
    VkRect2D scissor{{0,0}, ctx.swapChainExtent};
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
    vkCreatePipelineLayout(ctx.device, &layoutInfo, nullptr, &ctx.pipelineLayout3D);
    
    VkGraphicsPipelineCreateInfo pipelineInfo{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = stages;
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &raster;
    pipelineInfo.pMultisampleState = &multisample;
    pipelineInfo.pColorBlendState = &colorBlend;
    pipelineInfo.layout = ctx.pipelineLayout3D;
    pipelineInfo.renderPass = ctx.renderPass3D;
    pipelineInfo.subpass = 0;
    
    VkResult result = vkCreateGraphicsPipelines(ctx.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &ctx.pipeline3D);
    
    vkDestroyShaderModule(ctx.device, vertModule, nullptr);
    vkDestroyShaderModule(ctx.device, fragModule, nullptr);
    
    return result == VK_SUCCESS;
}

bool VkInit::createFramebuffers3D() {
    ctx.framebuffers3D.resize(ctx.swapChainImageViews.size());
    for (size_t i = 0; i < ctx.swapChainImageViews.size(); i++) {
        VkFramebufferCreateInfo fbInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        fbInfo.renderPass = ctx.renderPass3D;
        fbInfo.attachmentCount = 1;
        fbInfo.pAttachments = &ctx.swapChainImageViews[i];
        fbInfo.width = ctx.swapChainExtent.width;
        fbInfo.height = ctx.swapChainExtent.height;
        fbInfo.layers = 1;
        if (vkCreateFramebuffer(ctx.device, &fbInfo, nullptr, &ctx.framebuffers3D[i]) != VK_SUCCESS)
            return false;
    }
    return true;
}

bool VkInit::createRenderPassUI() {
    VkAttachmentDescription colorAtt{};
    colorAtt.format = ctx.swapChainImageFormat;
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
    
    return vkCreateRenderPass(ctx.device, &rpInfo, nullptr, &ctx.renderPassUI) == VK_SUCCESS;
}

bool VkInit::createPipelineUI() {
    auto vertCode = readFile("ui_vert.spv");
    auto fragCode = readFile("ui_frag.spv");
    if (vertCode.empty() || fragCode.empty()) {
        printf("[Vulkan] Failed to load UI shaders\n");
        return false;
    }
    
    VkShaderModule vertModule = createShaderModule(vertCode);
    VkShaderModule fragModule = createShaderModule(fragCode);
    if (!vertModule || !fragModule) return false;
    
    VkPipelineShaderStageCreateInfo stages[2] = {};
    stages[0] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertModule;
    stages[0].pName = "main";
    stages[1] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragModule;
    stages[1].pName = "main";
    
    VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = sizeof(float) * 6;
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    VkVertexInputAttributeDescription attributes[2];
    attributes[0] = {0, 0, VK_FORMAT_R32G32_SFLOAT, 0};
    attributes[1] = {1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, sizeof(float) * 2};
    
    VkPipelineVertexInputStateCreateInfo vertexInput{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &binding;
    vertexInput.vertexAttributeDescriptionCount = 2;
    vertexInput.pVertexAttributeDescriptions = attributes;
    
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    
    VkViewport viewport{0, 0, (float)ctx.swapChainExtent.width, (float)ctx.swapChainExtent.height, 0, 1};
    VkRect2D scissor{{0,0}, ctx.swapChainExtent};
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
    vkCreatePipelineLayout(ctx.device, &layoutInfo, nullptr, &ctx.pipelineLayoutUI);
    
    VkGraphicsPipelineCreateInfo pipelineInfo{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = stages;
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &raster;
    pipelineInfo.pMultisampleState = &multisample;
    pipelineInfo.pColorBlendState = &colorBlend;
    pipelineInfo.layout = ctx.pipelineLayoutUI;
    pipelineInfo.renderPass = ctx.renderPassUI;
    pipelineInfo.subpass = 0;
    
    VkResult result = vkCreateGraphicsPipelines(ctx.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &ctx.pipelineUI);
    
    vkDestroyShaderModule(ctx.device, vertModule, nullptr);
    vkDestroyShaderModule(ctx.device, fragModule, nullptr);
    
    return result == VK_SUCCESS;
}

bool VkInit::createFramebuffersUI() {
    ctx.framebuffersUI.resize(ctx.swapChainImageViews.size());
    for (size_t i = 0; i < ctx.swapChainImageViews.size(); i++) {
        VkFramebufferCreateInfo fbInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        fbInfo.renderPass = ctx.renderPassUI;
        fbInfo.attachmentCount = 1;
        fbInfo.pAttachments = &ctx.swapChainImageViews[i];
        fbInfo.width = ctx.swapChainExtent.width;
        fbInfo.height = ctx.swapChainExtent.height;
        fbInfo.layers = 1;
        if (vkCreateFramebuffer(ctx.device, &fbInfo, nullptr, &ctx.framebuffersUI[i]) != VK_SUCCESS)
            return false;
    }
    return true;
}

bool VkInit::initialize(HWND hwnd, int width, int height) {
    if (initialized) return true;
    
    ctx.hwnd = hwnd;
    ctx.width = width;
    ctx.height = height;
    
    if (!createInstance()) { printf("[Vulkan] Failed createInstance\n"); return false; }
    if (!createSurface()) { printf("[Vulkan] Failed createSurface\n"); return false; }
    if (!pickPhysicalDevice()) { printf("[Vulkan] Failed pickPhysicalDevice\n"); return false; }
    if (!createLogicalDevice()) { printf("[Vulkan] Failed createLogicalDevice\n"); return false; }
    if (!createSwapChain()) { printf("[Vulkan] Failed createSwapChain\n"); return false; }
    if (!createImageViews()) { printf("[Vulkan] Failed createImageViews\n"); return false; }
    if (!createCommandPool()) { printf("[Vulkan] Failed createCommandPool\n"); return false; }
    if (!createSemaphores()) { printf("[Vulkan] Failed createSemaphores\n"); return false; }
    if (!createVertexBuffers()) { printf("[Vulkan] Failed createVertexBuffers\n"); return false; }
    
    if (!createRenderPass3D()) { printf("[Vulkan] Failed createRenderPass3D\n"); return false; }
    if (!createPipeline3D()) { printf("[Vulkan] Failed createPipeline3D\n"); return false; }
    if (!createFramebuffers3D()) { printf("[Vulkan] Failed createFramebuffers3D\n"); return false; }
    
    if (!createRenderPassUI()) { printf("[Vulkan] Failed createRenderPassUI\n"); return false; }
    if (!createPipelineUI()) { printf("[Vulkan] Failed createPipelineUI\n"); return false; }
    if (!createFramebuffersUI()) { printf("[Vulkan] Failed createFramebuffersUI\n"); return false; }
    
    initialized = true;
    printf("[Vulkan] Initialized successfully!\n");
    printf("[Vulkan] initialize() - renderFrame will be called from GameLoop\n");
    return true;
}

void VkInit::cleanupSwapChain() {
    for (auto fb : ctx.framebuffers3D) vkDestroyFramebuffer(ctx.device, fb, nullptr);
    for (auto fb : ctx.framebuffersUI) vkDestroyFramebuffer(ctx.device, fb, nullptr);
    for (auto iv : ctx.swapChainImageViews) vkDestroyImageView(ctx.device, iv, nullptr);
    if (ctx.swapChain) vkDestroySwapchainKHR(ctx.device, ctx.swapChain, nullptr);
    ctx.framebuffers3D.clear();
    ctx.framebuffersUI.clear();
    ctx.swapChainImageViews.clear();
    ctx.swapChainImages.clear();
}

void VkInit::recreateSwapChain() {
    vkDeviceWaitIdle(ctx.device);
    cleanupSwapChain();
    
    RECT rect;
    GetClientRect(ctx.hwnd, &rect);
    ctx.width = rect.right - rect.left;
    ctx.height = rect.bottom - rect.top;
    
    createSwapChain();
    createImageViews();
    createFramebuffers3D();
    createFramebuffersUI();
}

void VkInit::cleanup() {
    if (!initialized) return;
    vkDeviceWaitIdle(ctx.device);
    
    cleanupSwapChain();
    
    if (ctx.pipeline3D) vkDestroyPipeline(ctx.device, ctx.pipeline3D, nullptr);
    if (ctx.pipelineLayout3D) vkDestroyPipelineLayout(ctx.device, ctx.pipelineLayout3D, nullptr);
    if (ctx.renderPass3D) vkDestroyRenderPass(ctx.device, ctx.renderPass3D, nullptr);
    
    if (ctx.pipelineUI) vkDestroyPipeline(ctx.device, ctx.pipelineUI, nullptr);
    if (ctx.pipelineLayoutUI) vkDestroyPipelineLayout(ctx.device, ctx.pipelineLayoutUI, nullptr);
    if (ctx.renderPassUI) vkDestroyRenderPass(ctx.device, ctx.renderPassUI, nullptr);
    
    if (ctx.vertexBuffer3D) vkDestroyBuffer(ctx.device, ctx.vertexBuffer3D, nullptr);
    if (ctx.vertexBufferMemory3D) vkFreeMemory(ctx.device, ctx.vertexBufferMemory3D, nullptr);
    if (ctx.vertexBufferUI) vkDestroyBuffer(ctx.device, ctx.vertexBufferUI, nullptr);
    if (ctx.vertexBufferMemoryUI) vkFreeMemory(ctx.device, ctx.vertexBufferMemoryUI, nullptr);
    
    if (ctx.commandPool) vkDestroyCommandPool(ctx.device, ctx.commandPool, nullptr);
    if (ctx.imageAvailableSemaphore) vkDestroySemaphore(ctx.device, ctx.imageAvailableSemaphore, nullptr);
    if (ctx.renderFinishedSemaphore) vkDestroySemaphore(ctx.device, ctx.renderFinishedSemaphore, nullptr);
    if (ctx.device) vkDestroyDevice(ctx.device, nullptr);
    if (ctx.surface) vkDestroySurfaceKHR(ctx.instance, ctx.surface, nullptr);
    if (ctx.instance) vkDestroyInstance(ctx.instance, nullptr);
    
    initialized = false;
}

bool VkInit::beginFrame(uint32_t& imageIndex) {
    VkResult result = vkAcquireNextImageKHR(ctx.device, ctx.swapChain, UINT64_MAX,
                                             ctx.imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return false;
    }
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) return false;
    
    vkResetCommandBuffer(ctx.commandBuffer, 0);
    
    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    if (vkBeginCommandBuffer(ctx.commandBuffer, &beginInfo) != VK_SUCCESS) return false;
    
    return true;
}

void VkInit::endFrame(uint32_t imageIndex) {
    vkEndCommandBuffer(ctx.commandBuffer);
    
    VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &ctx.imageAvailableSemaphore;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &ctx.commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &ctx.renderFinishedSemaphore;
    
    vkQueueSubmit(ctx.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    
    VkPresentInfoKHR presentInfo{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &ctx.renderFinishedSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &ctx.swapChain;
    presentInfo.pImageIndices = &imageIndex;
    
    vkQueuePresentKHR(ctx.graphicsQueue, &presentInfo);
    vkQueueWaitIdle(ctx.graphicsQueue);
}

void VkInit::renderFrame() {
    uint32_t imageIndex;
    if (!beginFrame(imageIndex)) return;
    printf("[Vulkan] renderFrame() CALLED - vertexCount3D=%u, vertexCountUI=%u\n", ctx.vertexCount3D, ctx.vertexCountUI);
    // 3D проход
    VkRenderPassBeginInfo rp3D{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    rp3D.renderPass = ctx.renderPass3D;
    rp3D.framebuffer = ctx.framebuffers3D[imageIndex];
    rp3D.renderArea.extent = ctx.swapChainExtent;
    VkClearValue clearColor = {0.1f, 0.1f, 0.2f, 1.0f};
    rp3D.clearValueCount = 1;
    rp3D.pClearValues = &clearColor;
    
    vkCmdBeginRenderPass(ctx.commandBuffer, &rp3D, VK_SUBPASS_CONTENTS_INLINE);
    if (ctx.vertexCount3D > 0) {
        vkCmdBindPipeline(ctx.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx.pipeline3D);
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(ctx.commandBuffer, 0, 1, &ctx.vertexBuffer3D, &offset);
        vkCmdDraw(ctx.commandBuffer, ctx.vertexCount3D, 1, 0, 0);
    }
    vkCmdEndRenderPass(ctx.commandBuffer);
    
    // UI проход поверх
    VkRenderPassBeginInfo rpUI{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    rpUI.renderPass = ctx.renderPassUI;
    rpUI.framebuffer = ctx.framebuffersUI[imageIndex];
    rpUI.renderArea.extent = ctx.swapChainExtent;
    rpUI.clearValueCount = 0;
    
    vkCmdBeginRenderPass(ctx.commandBuffer, &rpUI, VK_SUBPASS_CONTENTS_INLINE);
    if (ctx.vertexCountUI > 0) {
        vkCmdBindPipeline(ctx.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx.pipelineUI);
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(ctx.commandBuffer, 0, 1, &ctx.vertexBufferUI, &offset);
        vkCmdDraw(ctx.commandBuffer, ctx.vertexCountUI, 1, 0, 0);
    }
    vkCmdEndRenderPass(ctx.commandBuffer);
    
    endFrame(imageIndex);
    
    ctx.vertexCount3D = 0;
    ctx.vertexCountUI = 0;
}

void VkInit::set3DData(const void* data, uint32_t vertexCount, size_t vertexSize) {
    if (vertexCount > 0 && data) {
        ctx.vertexCount3D = vertexCount;
        ctx.vertexSize3D = vertexSize;
        void* mapped;
        vkMapMemory(ctx.device, ctx.vertexBufferMemory3D, 0, vertexCount * vertexSize, 0, &mapped);
        memcpy(mapped, data, vertexCount * vertexSize);
        vkUnmapMemory(ctx.device, ctx.vertexBufferMemory3D);
    }
}

void VkInit::setUIData(const void* data, uint32_t vertexCount, size_t vertexSize) {
    if (vertexCount > 0 && data) {
        ctx.vertexCountUI = vertexCount;
        ctx.vertexSizeUI = vertexSize;
        void* mapped;
        vkMapMemory(ctx.device, ctx.vertexBufferMemoryUI, 0, vertexCount * vertexSize, 0, &mapped);
        memcpy(mapped, data, vertexCount * vertexSize);
        vkUnmapMemory(ctx.device, ctx.vertexBufferMemoryUI);
    }
}