#include "Vulkan.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/scene.h>
#include "stb_image.h"

#pragma comment(lib, "vulkan-1.lib")

static std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) return {};
    size_t fileSize = file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    return buffer;
}

static void compileShaders() {
    system("mkdir autoshadertest 2>nul");
    
    const char* vertCode = "#version 450\nlayout(binding = 0) uniform UniformBufferObject { mat4 model; mat4 view; mat4 proj; } ubo; layout(location = 0) in vec3 inPosition; layout(location = 1) in vec3 inColor; layout(location = 2) in vec2 inTexCoord; layout(location = 0) out vec2 fragTexCoord; void main() { gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0); fragTexCoord = inTexCoord; }";
    const char* fragCode = "#version 450\nlayout(binding = 1) uniform sampler2D texSampler; layout(location = 0) in vec2 fragTexCoord; layout(location = 0) out vec4 outColor; void main() { outColor = texture(texSampler, fragTexCoord); }";
    const char* uiVertCode = "#version 450\nlayout(location = 0) in vec2 inPos; layout(location = 1) in vec3 inColor; layout(location = 0) out vec3 fragColor; void main() { gl_Position = vec4(inPos, 0.0, 1.0); fragColor = inColor; }";
    const char* uiFragCode = "#version 450\nlayout(location = 0) in vec3 fragColor; layout(location = 0) out vec4 outColor; void main() { outColor = vec4(fragColor, 1.0); }";
    const char* uiTextVertCode = "#version 450\nlayout(location = 0) in vec2 inPos; layout(location = 1) in vec2 inTexCoord; layout(location = 2) in vec3 inColor; layout(location = 0) out vec2 fragTexCoord; layout(location = 1) out vec3 fragColor; void main() { gl_Position = vec4(inPos, 0.0, 1.0); fragTexCoord = inTexCoord; fragColor = inColor; }";
    const char* uiTextFragCode = "#version 450\nlayout(binding = 0) uniform sampler2D fontSampler; layout(location = 0) in vec2 fragTexCoord; layout(location = 1) in vec3 fragColor; layout(location = 0) out vec4 outColor; void main() { vec4 texColor = texture(fontSampler, fragTexCoord); outColor = vec4(fragColor, texColor.a); }";
    const char* uiImageVertCode = "#version 450\nlayout(location = 0) in vec2 inPos; layout(location = 1) in vec2 inTexCoord; layout(location = 0) out vec2 fragTexCoord; void main() { gl_Position = vec4(inPos, 0.0, 1.0); fragTexCoord = inTexCoord; }";
    const char* uiImageFragCode = "#version 450\nlayout(binding = 0) uniform sampler2D imageSampler; layout(location = 0) in vec2 fragTexCoord; layout(location = 0) out vec4 outColor; void main() { outColor = texture(imageSampler, fragTexCoord); }";
    
    std::ofstream vertFile("autoshadertest/vert.vert"); vertFile << vertCode; vertFile.close();
    std::ofstream fragFile("autoshadertest/frag.frag"); fragFile << fragCode; fragFile.close();
    std::ofstream uiVertFile("autoshadertest/ui_vert.vert"); uiVertFile << uiVertCode; uiVertFile.close();
    std::ofstream uiFragFile("autoshadertest/ui_frag.frag"); uiFragFile << uiFragCode; uiFragFile.close();
    std::ofstream uiTextVertFile("autoshadertest/ui_text_vert.vert"); uiTextVertFile << uiTextVertCode; uiTextVertFile.close();
    std::ofstream uiTextFragFile("autoshadertest/ui_text_frag.frag"); uiTextFragFile << uiTextFragCode; uiTextFragFile.close();
    std::ofstream uiImageVertFile("autoshadertest/ui_image_vert.vert"); uiImageVertFile << uiImageVertCode; uiImageVertFile.close();
    std::ofstream uiImageFragFile("autoshadertest/ui_image_frag.frag"); uiImageFragFile << uiImageFragCode; uiImageFragFile.close();
    
    system("glslc autoshadertest/vert.vert -o autoshadertest/vert.spv");
    system("glslc autoshadertest/frag.frag -o autoshadertest/frag.spv");
    system("glslc autoshadertest/ui_vert.vert -o autoshadertest/ui_vert.spv");
    system("glslc autoshadertest/ui_frag.frag -o autoshadertest/ui_frag.spv");
    system("glslc autoshadertest/ui_text_vert.vert -o autoshadertest/ui_text_vert.spv");
    system("glslc autoshadertest/ui_text_frag.frag -o autoshadertest/ui_text_frag.spv");
    system("glslc autoshadertest/ui_image_vert.vert -o autoshadertest/ui_image_vert.spv");
    system("glslc autoshadertest/ui_image_frag.frag -o autoshadertest/ui_image_frag.spv");
}

bool Vulkan::initializeFont() {
    if (fontInitialized) return true;
    
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    char* lastSlash = strrchr(exePath, '\\');
    if (lastSlash) *lastSlash = '\0';
    
    char fontPath1[MAX_PATH], fontPath2[MAX_PATH], fontPath3[MAX_PATH], fontPath4[MAX_PATH];
    char fontPath5[MAX_PATH], fontPath6[MAX_PATH], fontPath7[MAX_PATH], fontPath8[MAX_PATH];
    char fontPath9[MAX_PATH], fontPath10[MAX_PATH];
    
    sprintf_s(fontPath1, "%s\\arial.ttf", exePath);
    sprintf_s(fontPath2, "%s\\fonts\\arial.ttf", exePath);
    sprintf_s(fontPath3, "%s\\tahoma.ttf", exePath);
    sprintf_s(fontPath4, "%s\\fonts\\tahoma.ttf", exePath);
    sprintf_s(fontPath5, "%s\\..\\fonts\\arial.ttf", exePath);
    sprintf_s(fontPath6, "%s\\..\\..\\fonts\\arial.ttf", exePath);
    sprintf_s(fontPath7, "fonts\\arial.ttf");
    sprintf_s(fontPath8, "arial.ttf");
    sprintf_s(fontPath9, "C:\\Windows\\Fonts\\arial.ttf");
    sprintf_s(fontPath10, "C:\\Windows\\Fonts\\tahoma.ttf");
    
    const char* fontPaths[] = {
        fontPath1, fontPath2, fontPath3, fontPath4, fontPath5,
        fontPath6, fontPath7, fontPath8, fontPath9, fontPath10
    };
    
    FILE* fontFile = nullptr;
    const char* loadedPath = nullptr;
    
    std::cout << "Searching for font files..." << std::endl;
    for (int i = 0; i < 10; i++) {
        fontFile = fopen(fontPaths[i], "rb");
        if (fontFile) {
            loadedPath = fontPaths[i];
            std::cout << "  SUCCESS! Font loaded from: " << loadedPath << std::endl;
            break;
        }
    }
    
    if (!fontFile) {
        std::cerr << "ERROR: Cannot load any font file" << std::endl;
        return false;
    }
    
    fseek(fontFile, 0, SEEK_END);
    long size = ftell(fontFile);
    fseek(fontFile, 0, SEEK_SET);
    
    unsigned char* fontBuffer = new unsigned char[size];
    fread(fontBuffer, 1, size, fontFile);
    fclose(fontFile);
    
    const int atlasWidth = 512;
    const int atlasHeight = 512;
    unsigned char* atlasBitmap = new unsigned char[atlasWidth * atlasHeight];
    memset(atlasBitmap, 0, atlasWidth * atlasHeight);
    
    int result = stbtt_BakeFontBitmap(fontBuffer, 0, 16.0f, atlasBitmap, 
                                       atlasWidth, atlasHeight, 32, 96, glyphs);
    
    if (result <= 0) {
        std::cerr << "ERROR: Failed to bake font bitmap" << std::endl;
        delete[] fontBuffer;
        delete[] atlasBitmap;
        return false;
    }
    
    unsigned char* rgbaBitmap = new unsigned char[atlasWidth * atlasHeight * 4];
    for (int i = 0; i < atlasWidth * atlasHeight; i++) {
        rgbaBitmap[i * 4 + 0] = 255;
        rgbaBitmap[i * 4 + 1] = 255;
        rgbaBitmap[i * 4 + 2] = 255;
        rgbaBitmap[i * 4 + 3] = atlasBitmap[i];
    }
    
    fontTexture = createTextureFromData(rgbaBitmap, atlasWidth, atlasHeight, 4);
    
    for (int i = 0; i < 96; i++) {
        char c = 32 + i;
        CharInfo info;
        info.u1 = glyphs[i].x0 / (float)atlasWidth;
        info.v1 = glyphs[i].y0 / (float)atlasHeight;
        info.u2 = glyphs[i].x1 / (float)atlasWidth;
        info.v2 = glyphs[i].y1 / (float)atlasHeight;
        info.advance = glyphs[i].xadvance;
        info.width = glyphs[i].x1 - glyphs[i].x0;
        info.height = glyphs[i].y1 - glyphs[i].y0;
        info.xoff = (float)glyphs[i].xoff;
        info.yoff = (float)glyphs[i].yoff;
        charMap[c] = info;
    }
    
    delete[] fontBuffer;
    delete[] atlasBitmap;
    delete[] rgbaBitmap;
    
    if (descPool != VK_NULL_HANDLE && descLayoutUIText != VK_NULL_HANDLE && fontTexture.valid) {
        VkDescriptorSetAllocateInfo descAlloc{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        descAlloc.descriptorPool = descPool;
        descAlloc.descriptorSetCount = 1;
        descAlloc.pSetLayouts = &descLayoutUIText;
        
        VkResult allocResult = vkAllocateDescriptorSets(device, &descAlloc, &descSetUIText);
        if (allocResult == VK_SUCCESS) {
            VkDescriptorImageInfo imageInfo{fontTexture.sampler, fontTexture.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            
            VkWriteDescriptorSet descriptorWrite{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            descriptorWrite.dstSet = descSetUIText;
            descriptorWrite.dstBinding = 0;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrite.pImageInfo = &imageInfo;
            
            vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
        }
    }
    
    createUITextBuffers();
    
    fontInitialized = true;
    std::cout << "Font initialized successfully" << std::endl;
    return true;
}

Vulkan::Vulkan(HWND hwnd, int width, int height) 
    : hwnd(hwnd), width(width), height(height), initialized(false), currentFrame(0), currentImageIndex(0), 
      swapchainImageCount(0), modelLoaded(false),
      depthImage(VK_NULL_HANDLE), depthImageMemory(VK_NULL_HANDLE), depthImageView(VK_NULL_HANDLE),
      instance(VK_NULL_HANDLE), physDevice(VK_NULL_HANDLE), device(VK_NULL_HANDLE),
      graphicsQueue(VK_NULL_HANDLE), surface(VK_NULL_HANDLE), swapchain(VK_NULL_HANDLE),
      renderPass(VK_NULL_HANDLE), pipelineLayout3D(VK_NULL_HANDLE), pipelineLayoutUI(VK_NULL_HANDLE),
      pipelineLayoutUIText(VK_NULL_HANDLE), pipelineLayoutUIImage(VK_NULL_HANDLE),
      pipeline3D(VK_NULL_HANDLE), pipelineUI(VK_NULL_HANDLE),
      pipelineUIText(VK_NULL_HANDLE), pipelineUIImage(VK_NULL_HANDLE),
      descLayout(VK_NULL_HANDLE), descLayoutUIEmpty(VK_NULL_HANDLE), descLayoutUIText(VK_NULL_HANDLE),
      descLayoutUIImage(VK_NULL_HANDLE),
      descPool(VK_NULL_HANDLE), descSetUIText(VK_NULL_HANDLE),
      vertexBuffer(VK_NULL_HANDLE), indexBuffer(VK_NULL_HANDLE),
      vertexBufferMemory(VK_NULL_HANDLE), indexBufferMemory(VK_NULL_HANDLE),
      uiVertexBuffer(VK_NULL_HANDLE), uiVertexBufferMemory(VK_NULL_HANDLE),
      uiTextVertexBuffer(VK_NULL_HANDLE), uiTextVertexBufferMemory(VK_NULL_HANDLE),
      uiImageVertexBuffer(VK_NULL_HANDLE), uiImageVertexBufferMemory(VK_NULL_HANDLE),
      fontInitialized(false) {
    
    memset(&fontTexture, 0, sizeof(fontTexture));
    memset(glyphs, 0, sizeof(glyphs));
    
    compileShaders();
    
    VkApplicationInfo appInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    appInfo.apiVersion = VK_API_VERSION_1_0;
    
    const char* exts[] = {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME};
    VkInstanceCreateInfo instInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instInfo.pApplicationInfo = &appInfo;
    instInfo.enabledExtensionCount = 2;
    instInfo.ppEnabledExtensionNames = exts;
    
    if (vkCreateInstance(&instInfo, nullptr, &instance) != VK_SUCCESS) {
        std::cerr << "Failed to create Vulkan instance" << std::endl;
        return;
    }
    
    VkWin32SurfaceCreateInfoKHR surfInfo{VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
    surfInfo.hinstance = GetModuleHandle(NULL);
    surfInfo.hwnd = hwnd;
    if (vkCreateWin32SurfaceKHR(instance, &surfInfo, nullptr, &surface) != VK_SUCCESS) {
        std::cerr << "Failed to create surface" << std::endl;
        return;
    }
    
    uint32_t deviceCount;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        std::cerr << "No Vulkan devices found" << std::endl;
        return;
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
    physDevice = devices[0];
    
    uint32_t queueFamily = 0;
    VkDeviceQueueCreateInfo queueInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queueInfo.queueFamilyIndex = queueFamily;
    queueInfo.queueCount = 1;
    float priority = 1.0f;
    queueInfo.pQueuePriorities = &priority;
    
    const char* devExts[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    VkDeviceCreateInfo devInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    devInfo.queueCreateInfoCount = 1;
    devInfo.pQueueCreateInfos = &queueInfo;
    devInfo.enabledExtensionCount = 1;
    devInfo.ppEnabledExtensionNames = devExts;
    
    if (vkCreateDevice(physDevice, &devInfo, nullptr, &device) != VK_SUCCESS) {
        std::cerr << "Failed to create logical device" << std::endl;
        return;
    }
    
    vkGetDeviceQueue(device, queueFamily, 0, &graphicsQueue);
    
    createSwapchain();
    createRenderPass();
    createFramebuffers();
    createMainDescriptorPool();
    createDescriptorSetLayout();
    createEmptyDescriptorSetLayout();
    createDescriptorSetLayoutUIText();
    createDescriptorSetLayoutUIImage();
    createPipelines();
    createUIImagePipeline();
    createUIBuffers();
    createUITextBuffers();
    createUIImageBuffers();
    
    frames.resize(MAX_FRAMES_IN_FLIGHT);
    createCommandPools();
    createUniformBuffers();
    createSyncObjects();
    
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkCommandBufferAllocateInfo allocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        allocInfo.commandPool = frames[i].commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;
        vkAllocateCommandBuffers(device, &allocInfo, &frames[i].cmdBuffer);
    }
    
    initializeFont();
    
    viewMat = glm::lookAt(glm::vec3(0.0f, 50.0f, 150.0f), glm::vec3(0, 50, 0), glm::vec3(0, 1, 0));
    projMat = glm::perspective(glm::radians(45.0f), (float)width/height, 0.1f, 1000.0f);
    projMat[1][1] *= -1;
    modelMat = glm::mat4(1.0f);
    
    initialized = true;
    std::cout << "Vulkan initialized successfully" << std::endl;
}

Vulkan::~Vulkan() {
    if (device) {
        vkDeviceWaitIdle(device);
        
        cleanupFrameResources();
        cleanupUITextures();
        
        for (auto& frame : frames) {
            vkDestroyCommandPool(device, frame.commandPool, nullptr);
        }
        
        vkDestroyPipeline(device, pipeline3D, nullptr);
        vkDestroyPipeline(device, pipelineUI, nullptr);
        vkDestroyPipeline(device, pipelineUIText, nullptr);
        vkDestroyPipeline(device, pipelineUIImage, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout3D, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayoutUI, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayoutUIText, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayoutUIImage, nullptr);
        vkDestroyRenderPass(device, renderPass, nullptr);
        vkDestroyDescriptorPool(device, descPool, nullptr);
        vkDestroyDescriptorSetLayout(device, descLayout, nullptr);
        vkDestroyDescriptorSetLayout(device, descLayoutUIEmpty, nullptr);
        vkDestroyDescriptorSetLayout(device, descLayoutUIText, nullptr);
        vkDestroyDescriptorSetLayout(device, descLayoutUIImage, nullptr);
        vkDestroyBuffer(device, vertexBuffer, nullptr);
        vkFreeMemory(device, vertexBufferMemory, nullptr);
        vkDestroyBuffer(device, indexBuffer, nullptr);
        vkFreeMemory(device, indexBufferMemory, nullptr);
        vkDestroyBuffer(device, uiVertexBuffer, nullptr);
        vkFreeMemory(device, uiVertexBufferMemory, nullptr);
        vkDestroyBuffer(device, uiTextVertexBuffer, nullptr);
        vkFreeMemory(device, uiTextVertexBufferMemory, nullptr);
        vkDestroyBuffer(device, uiImageVertexBuffer, nullptr);
        vkFreeMemory(device, uiImageVertexBufferMemory, nullptr);
        
        vkDestroyImageView(device, depthImageView, nullptr);
        vkDestroyImage(device, depthImage, nullptr);
        vkFreeMemory(device, depthImageMemory, nullptr);
        
        cleanupTextures();
        cleanupSwapchain();
        
        vkDestroyDevice(device, nullptr);
    }
    if (surface) vkDestroySurfaceKHR(instance, surface, nullptr);
    if (instance) vkDestroyInstance(instance, nullptr);
}

void Vulkan::createMainDescriptorPool() {
    VkDescriptorPoolSize poolSizes[] = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100}
    };
    
    VkDescriptorPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    poolInfo.poolSizeCount = 2;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = 100;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    
    vkCreateDescriptorPool(device, &poolInfo, nullptr, &descPool);
}

void Vulkan::cleanupFrameResources() {
    for (auto& frame : frames) {
        if (frame.uniformBuffer) vkDestroyBuffer(device, frame.uniformBuffer, nullptr);
        if (frame.uniformBufferMemory) vkFreeMemory(device, frame.uniformBufferMemory, nullptr);
        if (frame.renderFinishedSemaphore) vkDestroySemaphore(device, frame.renderFinishedSemaphore, nullptr);
        if (frame.imageAvailableSemaphore) vkDestroySemaphore(device, frame.imageAvailableSemaphore, nullptr);
        if (frame.inFlightFence) vkDestroyFence(device, frame.inFlightFence, nullptr);
    }
    frames.clear();
}

void Vulkan::cleanupSwapchain() {
    for (auto fb : framebuffers) vkDestroyFramebuffer(device, fb, nullptr);
    for (auto iv : swapchainImageViews) vkDestroyImageView(device, iv, nullptr);
    vkDestroySwapchainKHR(device, swapchain, nullptr);
}

void Vulkan::cleanupTextures() {
    for (auto& tex : meshTextures) {
        if (tex.valid) {
            vkDestroyImageView(device, tex.view, nullptr);
            vkDestroyImage(device, tex.image, nullptr);
            vkFreeMemory(device, tex.memory, nullptr);
            vkDestroySampler(device, tex.sampler, nullptr);
        }
    }
    meshTextures.clear();
    
    if (fontTexture.valid) {
        vkDestroyImageView(device, fontTexture.view, nullptr);
        vkDestroyImage(device, fontTexture.image, nullptr);
        vkFreeMemory(device, fontTexture.memory, nullptr);
        vkDestroySampler(device, fontTexture.sampler, nullptr);
        fontTexture.valid = false;
    }
}

void Vulkan::cleanupUITextures() {
    for (auto tex : loadedUITextures) {
        if (tex && tex->valid) {
            vkDestroyImageView(device, tex->view, nullptr);
            vkDestroyImage(device, tex->image, nullptr);
            vkFreeMemory(device, tex->memory, nullptr);
            vkDestroySampler(device, tex->sampler, nullptr);
            delete tex;
        }
    }
    loadedUITextures.clear();
}

uint32_t Vulkan::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(physDevice, &memProps);
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    return 0;
}

VkShaderModule Vulkan::createShaderModule(const std::string& filename) {
    auto code = readFile(filename);
    if (code.empty()) return VK_NULL_HANDLE;
    VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    info.codeSize = code.size();
    info.pCode = reinterpret_cast<const uint32_t*>(code.data());
    VkShaderModule module;
    vkCreateShaderModule(device, &info, nullptr, &module);
    return module;
}

VulkanTexture Vulkan::createTextureFromData(unsigned char* data, int width, int height, int channels) {
    VulkanTexture tex;
    tex.valid = false;
    tex.width = width;
    tex.height = height;
    if (!data) return tex;
    
    VkDeviceSize imageSize = width * height * 4;
    VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
    
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = imageSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    vkCreateBuffer(device, &bufferInfo, nullptr, &stagingBuffer);
    
    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(device, stagingBuffer, &memReq);
    VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkAllocateMemory(device, &allocInfo, nullptr, &stagingBufferMemory);
    vkBindBufferMemory(device, stagingBuffer, stagingBufferMemory, 0);
    
    void* mappedData;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &mappedData);
    memcpy(mappedData, data, imageSize);
    vkUnmapMemory(device, stagingBufferMemory);
    
    VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = (uint32_t)width;
    imageInfo.extent.height = (uint32_t)height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    vkCreateImage(device, &imageInfo, nullptr, &tex.image);
    
    vkGetImageMemoryRequirements(device, tex.image, &memReq);
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkAllocateMemory(device, &allocInfo, nullptr, &tex.memory);
    vkBindImageMemory(device, tex.image, tex.memory, 0);
    
    VkCommandBufferAllocateInfo cmdAllocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cmdAllocInfo.commandPool = frames[0].commandPool;
    cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAllocInfo.commandBufferCount = 1;
    VkCommandBuffer copyCmd;
    vkAllocateCommandBuffers(device, &cmdAllocInfo, &copyCmd);
    
    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(copyCmd, &beginInfo);
    
    VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = tex.image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    
    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent = {(uint32_t)width, (uint32_t)height, 1};
    vkCmdCopyBufferToImage(copyCmd, stagingBuffer, tex.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    
    vkEndCommandBuffer(copyCmd);
    
    VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &copyCmd;
    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);
    vkFreeCommandBuffers(device, frames[0].commandPool, 1, &copyCmd);
    
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
    
    VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    viewInfo.image = tex.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;
    vkCreateImageView(device, &viewInfo, nullptr, &tex.view);
    
    VkSamplerCreateInfo samplerInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    vkCreateSampler(device, &samplerInfo, nullptr, &tex.sampler);
    
    tex.valid = true;
    return tex;
}

VulkanTexture Vulkan::createWhiteTexture() {
    unsigned char white[] = {255, 255, 255, 255};
    return createTextureFromData(white, 1, 1, 4);
}

VulkanTexture* Vulkan::loadUIImage(const std::string& filepath) {
    int width, height, channels;
    unsigned char* data = stbi_load(filepath.c_str(), &width, &height, &channels, 4);
    
    if (!data) {
        std::cerr << "Failed to load image: " << filepath << std::endl;
        return nullptr;
    }
    
    VulkanTexture* texture = new VulkanTexture();
    *texture = createTextureFromData(data, width, height, 4);
    stbi_image_free(data);
    
    if (texture->valid) {
        loadedUITextures.push_back(texture);
        std::cout << "Loaded UI image: " << filepath << " (" << width << "x" << height << ")" << std::endl;
        return texture;
    }
    
    delete texture;
    return nullptr;
} 


void Vulkan::freeUIImage(VulkanTexture* texture) {
    if (!texture) return;
    
    auto it = std::find(loadedUITextures.begin(), loadedUITextures.end(), texture);
    if (it != loadedUITextures.end()) {
        loadedUITextures.erase(it);
    }
    
    if (texture->valid) {
        vkDeviceWaitIdle(device);
        vkDestroyImageView(device, texture->view, nullptr);
        vkDestroyImage(device, texture->image, nullptr);
        vkFreeMemory(device, texture->memory, nullptr);
        vkDestroySampler(device, texture->sampler, nullptr);
    }
    delete texture;
}

void Vulkan::drawImage(float x1, float y1, float x2, float y2, VulkanTexture* texture) {
    if (!texture || !texture->valid) return;
    drawImageUV(x1, y1, x2, y2, texture, 0.0f, 0.0f, 1.0f, 1.0f);
}

void Vulkan::drawImageUV(float x1, float y1, float x2, float y2, VulkanTexture* texture, 
                         float u1, float v1, float u2, float v2) {
    if (!texture || !texture->valid) return;
    
    UIImageQuad quad;
    quad.x1 = x1;
    quad.y1 = y1;
    quad.x2 = x2;
    quad.y2 = y2;
    quad.u1 = u1;
    quad.v1 = v1;
    quad.u2 = u2;
    quad.v2 = v2;
    quad.texture = texture;
    uiImageQuads.push_back(quad);
}

void Vulkan::createDescriptorSetLayoutUIImage() {
    VkDescriptorSetLayoutBinding samplerBinding{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
    
    VkDescriptorSetLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &samplerBinding;
    vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descLayoutUIImage);
}

void Vulkan::createUIImagePipeline() {
    VkShaderModule vertModule = createShaderModule("autoshadertest/ui_image_vert.spv");
    VkShaderModule fragModule = createShaderModule("autoshadertest/ui_image_frag.spv");
    
    if (vertModule == VK_NULL_HANDLE || fragModule == VK_NULL_HANDLE) {
        std::cerr << "Failed to load UI image shaders" << std::endl;
        if (vertModule) vkDestroyShaderModule(device, vertModule, nullptr);
        if (fragModule) vkDestroyShaderModule(device, fragModule, nullptr);
        return;
    }
    
    auto bindingDesc = VkVertexInputBindingDescription{0, sizeof(UIImageVertex), VK_VERTEX_INPUT_RATE_VERTEX};
    auto attrDesc = std::array<VkVertexInputAttributeDescription, 2>{
        VkVertexInputAttributeDescription{0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(UIImageVertex, pos)},
        VkVertexInputAttributeDescription{1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(UIImageVertex, texCoord)}
    };
    
    VkPipelineVertexInputStateCreateInfo vi{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vi.vertexBindingDescriptionCount = 1;
    vi.pVertexBindingDescriptions = &bindingDesc;
    vi.vertexAttributeDescriptionCount = (uint32_t)attrDesc.size();
    vi.pVertexAttributeDescriptions = attrDesc.data();
    
    VkPipelineInputAssemblyStateCreateInfo ia{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    
    VkViewport vp{0, 0, (float)swapchainExtent.width, (float)swapchainExtent.height, 0, 1};
    VkRect2D scissor{{0, 0}, {swapchainExtent.width, swapchainExtent.height}};
    VkPipelineViewportStateCreateInfo vpState{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    vpState.viewportCount = 1;
    vpState.pViewports = &vp;
    vpState.scissorCount = 1;
    vpState.pScissors = &scissor;
    
    VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.cullMode = VK_CULL_MODE_NONE;
    raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster.lineWidth = 1.0f;
    
    VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    
    VkPipelineDepthStencilStateCreateInfo depthStencil{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    depthStencil.depthTestEnable = VK_FALSE;
    depthStencil.depthWriteEnable = VK_FALSE;
    
    VkPipelineColorBlendAttachmentState blendAtt{};
    blendAtt.blendEnable = VK_TRUE;
    blendAtt.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendAtt.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAtt.colorBlendOp = VK_BLEND_OP_ADD;
    blendAtt.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blendAtt.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blendAtt.alphaBlendOp = VK_BLEND_OP_ADD;
    blendAtt.colorWriteMask = 0xF;
    
    VkPipelineColorBlendStateCreateInfo cb{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    cb.attachmentCount = 1;
    cb.pAttachments = &blendAtt;
    
    VkPipelineLayoutCreateInfo plInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    plInfo.setLayoutCount = 1;
    plInfo.pSetLayouts = &descLayoutUIImage;
    vkCreatePipelineLayout(device, &plInfo, nullptr, &pipelineLayoutUIImage);
    
    VkPipelineShaderStageCreateInfo stages[2] = {
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO},
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO}
    };
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertModule;
    stages[0].pName = "main";
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragModule;
    stages[1].pName = "main";
    
    VkGraphicsPipelineCreateInfo pipelineInfo{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = stages;
    pipelineInfo.pVertexInputState = &vi;
    pipelineInfo.pInputAssemblyState = &ia;
    pipelineInfo.pViewportState = &vpState;
    pipelineInfo.pRasterizationState = &raster;
    pipelineInfo.pMultisampleState = &ms;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &cb;
    pipelineInfo.layout = pipelineLayoutUIImage;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipelineUIImage);
    
    vkDestroyShaderModule(device, vertModule, nullptr);
    vkDestroyShaderModule(device, fragModule, nullptr);
    
    std::cout << "UI Image pipeline created" << std::endl;
}

void Vulkan::createUIImageBuffers() {
    VkDeviceSize bufferSize = sizeof(UIImageVertex) * 65536;
    
    VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    vkCreateBuffer(device, &bufferInfo, nullptr, &uiImageVertexBuffer);
    
    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(device, uiImageVertexBuffer, &memReq);
    VkMemoryAllocateInfo memAlloc{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    memAlloc.allocationSize = memReq.size;
    memAlloc.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkAllocateMemory(device, &memAlloc, nullptr, &uiImageVertexBufferMemory);
    vkBindBufferMemory(device, uiImageVertexBuffer, uiImageVertexBufferMemory, 0);
}

void Vulkan::renderUIImage() {
    if (uiImageQuads.empty()) return;
    
    vkCmdBindPipeline(frames[currentFrame].cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineUIImage);
    
    std::vector<UIImageVertex> vertices;
    VulkanTexture* currentTexture = nullptr;
    std::vector<std::pair<VulkanTexture*, std::vector<UIImageVertex>>> batches;
    
    for (const auto& quad : uiImageQuads) {
        float x1 = (quad.x1 / width) * 2.0f - 1.0f;
        float y1 = (quad.y1 / height) * 2.0f - 1.0f;
        float x2 = (quad.x2 / width) * 2.0f - 1.0f;
        float y2 = (quad.y2 / height) * 2.0f - 1.0f;
        
        std::vector<UIImageVertex> quadVerts = {
            {{x1, y1}, {quad.u1, quad.v1}},
            {{x2, y1}, {quad.u2, quad.v1}},
            {{x1, y2}, {quad.u1, quad.v2}},
            {{x2, y1}, {quad.u2, quad.v1}},
            {{x2, y2}, {quad.u2, quad.v2}},
            {{x1, y2}, {quad.u1, quad.v2}}
        };
        
        if (currentTexture == quad.texture) {
            vertices.insert(vertices.end(), quadVerts.begin(), quadVerts.end());
        } else {
            if (!vertices.empty()) {
                batches.push_back({currentTexture, vertices});
                vertices.clear();
            }
            currentTexture = quad.texture;
            vertices.insert(vertices.end(), quadVerts.begin(), quadVerts.end());
        }
    }
    
    if (!vertices.empty()) {
        batches.push_back({currentTexture, vertices});
    }
    
    for (const auto& batch : batches) {
        if (!batch.first || !batch.first->valid) continue;
        
        VkDescriptorSet descSet;
        VkDescriptorSetAllocateInfo descAlloc{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        descAlloc.descriptorPool = descPool;
        descAlloc.descriptorSetCount = 1;
        descAlloc.pSetLayouts = &descLayoutUIImage;
        
        if (vkAllocateDescriptorSets(device, &descAlloc, &descSet) != VK_SUCCESS) {
            continue;
        }
        
        VkDescriptorImageInfo imageInfo{batch.first->sampler, batch.first->view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        
        VkWriteDescriptorSet descriptorWrite{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        descriptorWrite.dstSet = descSet;
        descriptorWrite.dstBinding = 0;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.pImageInfo = &imageInfo;
        
        vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
        vkCmdBindDescriptorSets(frames[currentFrame].cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayoutUIImage, 0, 1, &descSet, 0, nullptr);
        
        void* data;
        vkMapMemory(device, uiImageVertexBufferMemory, 0, batch.second.size() * sizeof(UIImageVertex), 0, &data);
        memcpy(data, batch.second.data(), batch.second.size() * sizeof(UIImageVertex));
        vkUnmapMemory(device, uiImageVertexBufferMemory);
        
        VkDeviceSize offsets = 0;
        vkCmdBindVertexBuffers(frames[currentFrame].cmdBuffer, 0, 1, &uiImageVertexBuffer, &offsets);
        vkCmdDraw(frames[currentFrame].cmdBuffer, (uint32_t)batch.second.size(), 1, 0, 0);
        
        vkFreeDescriptorSets(device, descPool, 1, &descSet);
    }
    
    uiImageQuads.clear();
}

void Vulkan::loadModel(const std::vector<StandardMesh>& meshes) {
    vkDeviceWaitIdle(device);
    
    modelVertices.clear();
    modelIndices.clear();
    meshVertexOffsets.clear();
    meshIndexOffsets.clear();
    meshTextures.clear();
    
    size_t currVertexOffset = 0;
    size_t currIndexOffset = 0;
    
    for (const auto& mesh : meshes) {
        meshVertexOffsets.push_back(currVertexOffset);
        meshIndexOffsets.push_back(currIndexOffset);
        
        for (const auto& vert : mesh.vertices) {
            VertexGPU v;
            v.pos = glm::vec3(vert.position[0], vert.position[1], vert.position[2]);
            v.color = glm::vec3(1.0f, 1.0f, 1.0f);
            v.texCoord = glm::vec2(vert.texCoords[0], vert.texCoords[1]);
            modelVertices.push_back(v);
        }
        
        for (unsigned int idx : mesh.indices) {
            modelIndices.push_back((uint32_t)(currVertexOffset + idx));
        }
        
        currVertexOffset += mesh.vertices.size();
        currIndexOffset += mesh.indices.size();
    }
    
    if (modelVertices.empty()) {
        std::cerr << "No vertices in model!" << std::endl;
        return;
    }
    
    for (size_t i = 0; i < meshes.size(); i++) {
        const auto& mesh = meshes[i];
        bool textureLoaded = false;
        
        for (const auto& texData : mesh.textures) {
            if (texData.type == "texture_diffuse" && texData.rawData.isValid && texData.rawData.data) {
                VulkanTexture vulkanTex = createTextureFromData(
                    texData.rawData.data,
                    texData.rawData.width,
                    texData.rawData.height,
                    texData.rawData.channels
                );
                
                if (vulkanTex.valid) {
                    meshTextures.push_back(vulkanTex);
                    textureLoaded = true;
                    break;
                }
            }
        }
        
        if (!textureLoaded) {
            meshTextures.push_back(createWhiteTexture());
        }
    }
    
    createModelBuffers();
    createDescriptorSetsForModel();
    
    modelLoaded = true;
    std::cout << "Model loaded: " << modelVertices.size() << " vertices" << std::endl;
}

void Vulkan::createModelBuffers() {
    if (modelVertices.empty()) return;
    
    VkDeviceSize vertSize = sizeof(VertexGPU) * modelVertices.size();
    VkBufferCreateInfo bufInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufInfo.size = vertSize;
    bufInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    vkCreateBuffer(device, &bufInfo, nullptr, &vertexBuffer);
    
    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(device, vertexBuffer, &memReq);
    VkMemoryAllocateInfo memAlloc{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    memAlloc.allocationSize = memReq.size;
    memAlloc.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkAllocateMemory(device, &memAlloc, nullptr, &vertexBufferMemory);
    vkBindBufferMemory(device, vertexBuffer, vertexBufferMemory, 0);
    
    void* data;
    vkMapMemory(device, vertexBufferMemory, 0, vertSize, 0, &data);
    memcpy(data, modelVertices.data(), vertSize);
    vkUnmapMemory(device, vertexBufferMemory);
    
    VkDeviceSize idxSize = sizeof(uint32_t) * modelIndices.size();
    bufInfo.size = idxSize;
    bufInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    vkCreateBuffer(device, &bufInfo, nullptr, &indexBuffer);
    vkGetBufferMemoryRequirements(device, indexBuffer, &memReq);
    memAlloc.allocationSize = memReq.size;
    memAlloc.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkAllocateMemory(device, &memAlloc, nullptr, &indexBufferMemory);
    vkBindBufferMemory(device, indexBuffer, indexBufferMemory, 0);
    vkMapMemory(device, indexBufferMemory, 0, idxSize, 0, &data);
    memcpy(data, modelIndices.data(), idxSize);
    vkUnmapMemory(device, indexBufferMemory);
}

void Vulkan::createCommandPools() {
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = 0;
        vkCreateCommandPool(device, &poolInfo, nullptr, &frames[i].commandPool);
    }
}

void Vulkan::createUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        bufferInfo.size = bufferSize;
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        vkCreateBuffer(device, &bufferInfo, nullptr, &frames[i].uniformBuffer);
        
        VkMemoryRequirements memReq;
        vkGetBufferMemoryRequirements(device, frames[i].uniformBuffer, &memReq);
        VkMemoryAllocateInfo memAlloc{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        memAlloc.allocationSize = memReq.size;
        memAlloc.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        vkAllocateMemory(device, &memAlloc, nullptr, &frames[i].uniformBufferMemory);
        vkBindBufferMemory(device, frames[i].uniformBuffer, frames[i].uniformBufferMemory, 0);
    }
}

void Vulkan::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uboBinding{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr};
    VkDescriptorSetLayoutBinding samplerBinding{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboBinding, samplerBinding};
    
    VkDescriptorSetLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layoutInfo.bindingCount = (uint32_t)bindings.size();
    layoutInfo.pBindings = bindings.data();
    vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descLayout);
}

void Vulkan::createEmptyDescriptorSetLayout() {
    VkDescriptorSetLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layoutInfo.bindingCount = 0;
    layoutInfo.pBindings = nullptr;
    vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descLayoutUIEmpty);
}

void Vulkan::createDescriptorSetLayoutUIText() {
    VkDescriptorSetLayoutBinding samplerBinding{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
    
    VkDescriptorSetLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &samplerBinding;
    vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descLayoutUIText);
}

void Vulkan::createDescriptorSetsForModel() {
    descSets.clear();
    
    if (meshTextures.empty()) {
        meshTextures.push_back(createWhiteTexture());
    }
    
    descSets.resize(meshTextures.size());
    
    for (size_t i = 0; i < meshTextures.size(); i++) {
        VkDescriptorSetAllocateInfo descAlloc{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        descAlloc.descriptorPool = descPool;
        descAlloc.descriptorSetCount = 1;
        descAlloc.pSetLayouts = &descLayout;
        vkAllocateDescriptorSets(device, &descAlloc, &descSets[i]);
        
        VkDescriptorImageInfo imageInfo{meshTextures[i].sampler, meshTextures[i].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkDescriptorBufferInfo bufInfo{frames[0].uniformBuffer, 0, sizeof(UniformBufferObject)};
        
        std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};
        descriptorWrites[0] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        descriptorWrites[0].dstSet = descSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].pBufferInfo = &bufInfo;
        
        descriptorWrites[1] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        descriptorWrites[1].dstSet = descSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].pImageInfo = &imageInfo;
        
        vkUpdateDescriptorSets(device, (uint32_t)descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
    }
}

void Vulkan::createSwapchain() {
    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDevice, surface, &caps);
    
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physDevice, surface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physDevice, surface, &formatCount, formats.data());
    
    VkSurfaceFormatKHR selectedFormat = formats[0];
    for (const auto& fmt : formats) {
        if (fmt.format == VK_FORMAT_B8G8R8A8_UNORM && fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            selectedFormat = fmt;
            break;
        }
    }
    swapchainFormat = selectedFormat.format;
    
    RECT rect;
    GetClientRect(hwnd, &rect);
    int clientWidth = rect.right - rect.left;
    int clientHeight = rect.bottom - rect.top;
    if (clientWidth <= 0) clientWidth = width;
    if (clientHeight <= 0) clientHeight = height;
    
    if (caps.currentExtent.width != 0xFFFFFFFF) {
        swapchainExtent = caps.currentExtent;
    } else {
        swapchainExtent.width = std::clamp((uint32_t)clientWidth, caps.minImageExtent.width, caps.maxImageExtent.width);
        swapchainExtent.height = std::clamp((uint32_t)clientHeight, caps.minImageExtent.height, caps.maxImageExtent.height);
    }
    
    uint32_t imageCount = std::clamp(caps.minImageCount + 1, caps.minImageCount, caps.maxImageCount > 0 ? caps.maxImageCount : caps.minImageCount + 1);
    
    VkSwapchainCreateInfoKHR swapInfo{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    swapInfo.surface = surface;
    swapInfo.minImageCount = imageCount;
    swapInfo.imageFormat = selectedFormat.format;
    swapInfo.imageColorSpace = selectedFormat.colorSpace;
    swapInfo.imageExtent = swapchainExtent;
    swapInfo.imageArrayLayers = 1;
    swapInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapInfo.preTransform = caps.currentTransform;
    swapInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapInfo.clipped = VK_TRUE;
    
    if (vkCreateSwapchainKHR(device, &swapInfo, nullptr, &swapchain) != VK_SUCCESS) {
        std::cerr << "Failed to create swapchain" << std::endl;
        return;
    }
    
    vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, nullptr);
    swapchainImages.resize(swapchainImageCount);
    vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, swapchainImages.data());
    
    swapchainImageViews.resize(swapchainImageCount);
    for (uint32_t i = 0; i < swapchainImageCount; i++) {
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

void Vulkan::createRenderPass() {
    VkAttachmentDescription attachments[2];
    
    attachments[0].flags = 0;
    attachments[0].format = swapchainFormat;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    
    attachments[1].flags = 0;
    attachments[1].format = VK_FORMAT_D32_SFLOAT;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    VkAttachmentReference colorRef = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentReference depthRef = {1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;
    subpass.pDepthStencilAttachment = &depthRef;
    
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    
    VkRenderPassCreateInfo rpInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    rpInfo.attachmentCount = 2;
    rpInfo.pAttachments = attachments;
    rpInfo.subpassCount = 1;
    rpInfo.pSubpasses = &subpass;
    rpInfo.dependencyCount = 1;
    rpInfo.pDependencies = &dependency;
    
    vkCreateRenderPass(device, &rpInfo, nullptr, &renderPass);
}

void Vulkan::createFramebuffers() {
    VkImageCreateInfo depthInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    depthInfo.flags = 0;
    depthInfo.imageType = VK_IMAGE_TYPE_2D;
    depthInfo.extent.width = swapchainExtent.width;
    depthInfo.extent.height = swapchainExtent.height;
    depthInfo.extent.depth = 1;
    depthInfo.mipLevels = 1;
    depthInfo.arrayLayers = 1;
    depthInfo.format = VK_FORMAT_D32_SFLOAT;
    depthInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    depthInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    depthInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    vkCreateImage(device, &depthInfo, nullptr, &depthImage);
    
    VkMemoryRequirements memReq;
    vkGetImageMemoryRequirements(device, depthImage, &memReq);
    VkMemoryAllocateInfo memAlloc{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    memAlloc.allocationSize = memReq.size;
    memAlloc.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkAllocateMemory(device, &memAlloc, nullptr, &depthImageMemory);
    vkBindImageMemory(device, depthImage, depthImageMemory, 0);
    
    VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    viewInfo.image = depthImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_D32_SFLOAT;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;
    vkCreateImageView(device, &viewInfo, nullptr, &depthImageView);
    
    framebuffers.resize(swapchainImageViews.size());
    for (size_t i = 0; i < swapchainImageViews.size(); i++) {
        std::array<VkImageView, 2> attachments = {swapchainImageViews[i], depthImageView};
        
        VkFramebufferCreateInfo fbInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        fbInfo.renderPass = renderPass;
        fbInfo.attachmentCount = 2;
        fbInfo.pAttachments = attachments.data();
        fbInfo.width = swapchainExtent.width;
        fbInfo.height = swapchainExtent.height;
        fbInfo.layers = 1;
        vkCreateFramebuffer(device, &fbInfo, nullptr, &framebuffers[i]);
    }
}

void Vulkan::createSyncObjects() {
    VkSemaphoreCreateInfo semInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkCreateSemaphore(device, &semInfo, nullptr, &frames[i].imageAvailableSemaphore);
        vkCreateSemaphore(device, &semInfo, nullptr, &frames[i].renderFinishedSemaphore);
        vkCreateFence(device, &fenceInfo, nullptr, &frames[i].inFlightFence);
    }
}

void Vulkan::createPipelines() {
    VkShaderModule vertModule = createShaderModule("autoshadertest/vert.spv");
    VkShaderModule fragModule = createShaderModule("autoshadertest/frag.spv");
    VkShaderModule uiVertModule = createShaderModule("autoshadertest/ui_vert.spv");
    VkShaderModule uiFragModule = createShaderModule("autoshadertest/ui_frag.spv");
    VkShaderModule uiTextVertModule = createShaderModule("autoshadertest/ui_text_vert.spv");
    VkShaderModule uiTextFragModule = createShaderModule("autoshadertest/ui_text_frag.spv");
    
    auto bindingDesc = VkVertexInputBindingDescription{0, sizeof(VertexGPU), VK_VERTEX_INPUT_RATE_VERTEX};
    auto attrDesc = std::array<VkVertexInputAttributeDescription, 3>{
        VkVertexInputAttributeDescription{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexGPU, pos)},
        VkVertexInputAttributeDescription{1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexGPU, color)},
        VkVertexInputAttributeDescription{2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexGPU, texCoord)}
    };
    
    VkPipelineVertexInputStateCreateInfo vi{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vi.vertexBindingDescriptionCount = 1;
    vi.pVertexBindingDescriptions = &bindingDesc;
    vi.vertexAttributeDescriptionCount = (uint32_t)attrDesc.size();
    vi.pVertexAttributeDescriptions = attrDesc.data();
    
    VkPipelineInputAssemblyStateCreateInfo ia{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    
    VkViewport vp{0, 0, (float)swapchainExtent.width, (float)swapchainExtent.height, 0, 1};
    VkRect2D scissor{{0, 0}, {swapchainExtent.width, swapchainExtent.height}};
    VkPipelineViewportStateCreateInfo vpState{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    vpState.viewportCount = 1;
    vpState.pViewports = &vp;
    vpState.scissorCount = 1;
    vpState.pScissors = &scissor;
    
    VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.cullMode = VK_CULL_MODE_NONE;
    raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster.lineWidth = 1.0f;
    
    VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    
    VkPipelineDepthStencilStateCreateInfo depthStencil{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;
    
    VkPipelineColorBlendAttachmentState blendAtt{};
    blendAtt.colorWriteMask = 0xF;
    VkPipelineColorBlendStateCreateInfo cb{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    cb.attachmentCount = 1;
    cb.pAttachments = &blendAtt;
    
    VkPipelineLayoutCreateInfo plInfo3D{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    plInfo3D.setLayoutCount = 1;
    plInfo3D.pSetLayouts = &descLayout;
    vkCreatePipelineLayout(device, &plInfo3D, nullptr, &pipelineLayout3D);
    
    VkPipelineShaderStageCreateInfo stages[2] = {
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO},
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO}
    };
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertModule;
    stages[0].pName = "main";
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragModule;
    stages[1].pName = "main";
    
    VkGraphicsPipelineCreateInfo pipelineInfo{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = stages;
    pipelineInfo.pVertexInputState = &vi;
    pipelineInfo.pInputAssemblyState = &ia;
    pipelineInfo.pViewportState = &vpState;
    pipelineInfo.pRasterizationState = &raster;
    pipelineInfo.pMultisampleState = &ms;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &cb;
    pipelineInfo.layout = pipelineLayout3D;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline3D);
    
    auto uiBindingDesc = VkVertexInputBindingDescription{0, sizeof(UIVertex), VK_VERTEX_INPUT_RATE_VERTEX};
    auto uiAttrDesc = std::array<VkVertexInputAttributeDescription, 2>{
        VkVertexInputAttributeDescription{0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(UIVertex, pos)},
        VkVertexInputAttributeDescription{1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(UIVertex, color)}
    };
    
    VkPipelineVertexInputStateCreateInfo uiVi{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    uiVi.vertexBindingDescriptionCount = 1;
    uiVi.pVertexBindingDescriptions = &uiBindingDesc;
    uiVi.vertexAttributeDescriptionCount = (uint32_t)uiAttrDesc.size();
    uiVi.pVertexAttributeDescriptions = uiAttrDesc.data();
    
    VkPipelineInputAssemblyStateCreateInfo uiIa{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    uiIa.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    
    VkPipelineColorBlendAttachmentState uiBlendAtt{};
    uiBlendAtt.blendEnable = VK_TRUE;
    uiBlendAtt.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    uiBlendAtt.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    uiBlendAtt.colorBlendOp = VK_BLEND_OP_ADD;
    uiBlendAtt.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    uiBlendAtt.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    uiBlendAtt.alphaBlendOp = VK_BLEND_OP_ADD;
    uiBlendAtt.colorWriteMask = 0xF;
    
    VkPipelineColorBlendStateCreateInfo uiCb{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    uiCb.attachmentCount = 1;
    uiCb.pAttachments = &uiBlendAtt;
    
    VkPipelineDepthStencilStateCreateInfo uiDepthStencil{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    uiDepthStencil.depthTestEnable = VK_FALSE;
    uiDepthStencil.depthWriteEnable = VK_FALSE;
    
    VkPipelineLayoutCreateInfo plInfoUI{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    plInfoUI.setLayoutCount = 1;
    plInfoUI.pSetLayouts = &descLayoutUIEmpty;
    vkCreatePipelineLayout(device, &plInfoUI, nullptr, &pipelineLayoutUI);
    
    stages[0].module = uiVertModule;
    stages[1].module = uiFragModule;
    
    VkGraphicsPipelineCreateInfo uiPipelineInfo{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    uiPipelineInfo.stageCount = 2;
    uiPipelineInfo.pStages = stages;
    uiPipelineInfo.pVertexInputState = &uiVi;
    uiPipelineInfo.pInputAssemblyState = &uiIa;
    uiPipelineInfo.pViewportState = &vpState;
    uiPipelineInfo.pRasterizationState = &raster;
    uiPipelineInfo.pMultisampleState = &ms;
    uiPipelineInfo.pDepthStencilState = &uiDepthStencil;
    uiPipelineInfo.pColorBlendState = &uiCb;
    uiPipelineInfo.layout = pipelineLayoutUI;
    uiPipelineInfo.renderPass = renderPass;
    uiPipelineInfo.subpass = 0;
    vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &uiPipelineInfo, nullptr, &pipelineUI);
    
    auto uiTextBindingDesc = VkVertexInputBindingDescription{0, sizeof(UITextVertex), VK_VERTEX_INPUT_RATE_VERTEX};
    auto uiTextAttrDesc = std::array<VkVertexInputAttributeDescription, 3>{
        VkVertexInputAttributeDescription{0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(UITextVertex, pos)},
        VkVertexInputAttributeDescription{1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(UITextVertex, texCoord)},
        VkVertexInputAttributeDescription{2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(UITextVertex, color)}
    };
    
    VkPipelineVertexInputStateCreateInfo uiTextVi{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    uiTextVi.vertexBindingDescriptionCount = 1;
    uiTextVi.pVertexBindingDescriptions = &uiTextBindingDesc;
    uiTextVi.vertexAttributeDescriptionCount = (uint32_t)uiTextAttrDesc.size();
    uiTextVi.pVertexAttributeDescriptions = uiTextAttrDesc.data();
    
    VkPipelineLayoutCreateInfo plInfoUIText{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    plInfoUIText.setLayoutCount = 1;
    plInfoUIText.pSetLayouts = &descLayoutUIText;
    vkCreatePipelineLayout(device, &plInfoUIText, nullptr, &pipelineLayoutUIText);
    
    stages[0].module = uiTextVertModule;
    stages[1].module = uiTextFragModule;
    
    VkGraphicsPipelineCreateInfo uiTextPipelineInfo{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    uiTextPipelineInfo.stageCount = 2;
    uiTextPipelineInfo.pStages = stages;
    uiTextPipelineInfo.pVertexInputState = &uiTextVi;
    uiTextPipelineInfo.pInputAssemblyState = &uiIa;
    uiTextPipelineInfo.pViewportState = &vpState;
    uiTextPipelineInfo.pRasterizationState = &raster;
    uiTextPipelineInfo.pMultisampleState = &ms;
    uiTextPipelineInfo.pDepthStencilState = &uiDepthStencil;
    uiTextPipelineInfo.pColorBlendState = &uiCb;
    uiTextPipelineInfo.layout = pipelineLayoutUIText;
    uiTextPipelineInfo.renderPass = renderPass;
    uiTextPipelineInfo.subpass = 0;
    vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &uiTextPipelineInfo, nullptr, &pipelineUIText);
    
    vkDestroyShaderModule(device, vertModule, nullptr);
    vkDestroyShaderModule(device, fragModule, nullptr);
    vkDestroyShaderModule(device, uiVertModule, nullptr);
    vkDestroyShaderModule(device, uiFragModule, nullptr);
    vkDestroyShaderModule(device, uiTextVertModule, nullptr);
    vkDestroyShaderModule(device, uiTextFragModule, nullptr);
}

void Vulkan::createUIBuffers() {
    VkDeviceSize bufferSize = sizeof(UIVertex) * 65536;
    
    VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    vkCreateBuffer(device, &bufferInfo, nullptr, &uiVertexBuffer);
    
    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(device, uiVertexBuffer, &memReq);
    VkMemoryAllocateInfo memAlloc{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    memAlloc.allocationSize = memReq.size;
    memAlloc.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkAllocateMemory(device, &memAlloc, nullptr, &uiVertexBufferMemory);
    vkBindBufferMemory(device, uiVertexBuffer, uiVertexBufferMemory, 0);
}

void Vulkan::createUITextBuffers() {
    VkDeviceSize bufferSize = sizeof(UITextVertex) * 65536;
    
    VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    vkCreateBuffer(device, &bufferInfo, nullptr, &uiTextVertexBuffer);
    
    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(device, uiTextVertexBuffer, &memReq);
    VkMemoryAllocateInfo memAlloc{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    memAlloc.allocationSize = memReq.size;
    memAlloc.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkAllocateMemory(device, &memAlloc, nullptr, &uiTextVertexBufferMemory);
    vkBindBufferMemory(device, uiTextVertexBuffer, uiTextVertexBufferMemory, 0);
}

void Vulkan::updateUniformBuffer(uint32_t frameIndex) {
    UniformBufferObject ubo;
    ubo.model = modelMat;
    ubo.view = viewMat;
    ubo.proj = projMat;
    
    void* data;
    vkMapMemory(device, frames[frameIndex].uniformBufferMemory, 0, sizeof(ubo), 0, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(device, frames[frameIndex].uniformBufferMemory);
}

void Vulkan::setup2D(int width, int height) {
    this->width = width;
    this->height = height;
    uiQuads.clear();
    uiTextQuads.clear();
    uiImageQuads.clear();
}

void Vulkan::drawQuad(float x1, float y1, float x2, float y2, float r, float g, float b) {
    UIQuad quad;
    quad.x1 = x1; quad.y1 = y1;
    quad.x2 = x2; quad.y2 = y2;
    quad.color = glm::vec3(r, g, b);
    uiQuads.push_back(quad);
}

float Vulkan::getTextWidth(const std::string& text) {
    float width = 0;
    for (char c : text) {
        auto it = charMap.find(c);
        if (it != charMap.end()) {
            width += it->second.advance;
        }
    }
    return width;
}

void Vulkan::drawText(int x, int y, const std::string& text, float r, float g, float b) {
    if (!fontInitialized) return;
    
    float curX = (float)x;
    float curY = (float)y;
    glm::vec3 color(r, g, b);
    
    for (char c : text) {
        auto it = charMap.find(c);
        if (it == charMap.end()) continue;
        
        CharInfo& ch = it->second;
        
        if (ch.width == 0 || ch.height == 0) {
            curX += ch.advance;
            continue;
        }
        
        float x0 = curX + ch.xoff;
        float y0 = curY + ch.yoff;
        float x1 = x0 + ch.width;
        float y1 = y0 + ch.height;
        
        UITextQuad quad;
        quad.x1 = x0;
        quad.y1 = y0;
        quad.x2 = x1;
        quad.y2 = y1;
        quad.u1 = ch.u1;
        quad.v1 = ch.v1;
        quad.u2 = ch.u2;
        quad.v2 = ch.v2;
        quad.color = color;
        uiTextQuads.push_back(quad);
        
        curX += ch.advance;
    }
}

void Vulkan::drawTextCentered(int x, int y, int w, int h, const std::string& text, float r, float g, float b) {
    if (!fontInitialized) return;
    
    float textWidth = getTextWidth(text);
    float textHeight = 16.0f;
    
    int centerX = x + (int)((w - textWidth) / 2);
    int centerY = y + (int)((h - textHeight) / 2);
    
    drawText(centerX, centerY, text, r, g, b);
}

void Vulkan::renderModel() {
    if (!modelLoaded || modelVertices.empty()) return;
    
    vkCmdBindPipeline(frames[currentFrame].cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline3D);
    VkDeviceSize offsets = 0;
    vkCmdBindVertexBuffers(frames[currentFrame].cmdBuffer, 0, 1, &vertexBuffer, &offsets);
    vkCmdBindIndexBuffer(frames[currentFrame].cmdBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    
    for (size_t i = 0; i < descSets.size() && i < meshTextures.size(); i++) {
        uint32_t indexCount = 0;
        if (i < meshIndexOffsets.size()) {
            uint32_t nextOffset = (i + 1 < meshIndexOffsets.size()) ? (uint32_t)meshIndexOffsets[i + 1] : (uint32_t)modelIndices.size();
            indexCount = nextOffset - (uint32_t)meshIndexOffsets[i];
        }
        
        if (indexCount > 0 && i < descSets.size()) {
            vkCmdBindDescriptorSets(frames[currentFrame].cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout3D, 0, 1, &descSets[i], 0, nullptr);
            vkCmdDrawIndexed(frames[currentFrame].cmdBuffer, indexCount, 1, (uint32_t)meshIndexOffsets[i], (uint32_t)meshVertexOffsets[i], 0);
        }
    }
}

void Vulkan::setViewMatrix(const glm::mat4& view) { viewMat = view; }
void Vulkan::setProjectionMatrix(const glm::mat4& proj) { projMat = proj; projMat[1][1] *= -1; }
void Vulkan::setModelMatrix(const glm::mat4& model) { modelMat = model; }

void Vulkan::renderUI() {
    if (uiQuads.empty()) return;
    
    vkCmdBindPipeline(frames[currentFrame].cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineUI);
    
    std::vector<UIVertex> vertices;
    for (const auto& quad : uiQuads) {
        float x1 = (quad.x1 / width) * 2.0f - 1.0f;
        float y1 = (quad.y1 / height) * 2.0f - 1.0f;
        float x2 = (quad.x2 / width) * 2.0f - 1.0f;
        float y2 = (quad.y2 / height) * 2.0f - 1.0f;
        
        vertices.push_back({{x1, y1}, quad.color});
        vertices.push_back({{x2, y1}, quad.color});
        vertices.push_back({{x1, y2}, quad.color});
        vertices.push_back({{x2, y1}, quad.color});
        vertices.push_back({{x2, y2}, quad.color});
        vertices.push_back({{x1, y2}, quad.color});
    }
    
    if (!vertices.empty()) {
        void* data;
        vkMapMemory(device, uiVertexBufferMemory, 0, vertices.size() * sizeof(UIVertex), 0, &data);
        memcpy(data, vertices.data(), vertices.size() * sizeof(UIVertex));
        vkUnmapMemory(device, uiVertexBufferMemory);
        
        VkDeviceSize offsets = 0;
        vkCmdBindVertexBuffers(frames[currentFrame].cmdBuffer, 0, 1, &uiVertexBuffer, &offsets);
        vkCmdDraw(frames[currentFrame].cmdBuffer, (uint32_t)vertices.size(), 1, 0, 0);
    }
    
    uiQuads.clear();
}

void Vulkan::renderUIText() {
    if (uiTextQuads.empty() || !fontInitialized || descSetUIText == VK_NULL_HANDLE) {
        uiTextQuads.clear();
        return;
    }
    
    vkCmdBindPipeline(frames[currentFrame].cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineUIText);
    vkCmdBindDescriptorSets(frames[currentFrame].cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayoutUIText, 0, 1, &descSetUIText, 0, nullptr);
    
    std::vector<UITextVertex> vertices;
    for (const auto& quad : uiTextQuads) {
        float x1 = (quad.x1 / width) * 2.0f - 1.0f;
        float y1 = (quad.y1 / height) * 2.0f - 1.0f;
        float x2 = (quad.x2 / width) * 2.0f - 1.0f;
        float y2 = (quad.y2 / height) * 2.0f - 1.0f;
        
        vertices.push_back({{x1, y1}, {quad.u1, quad.v1}, quad.color});
        vertices.push_back({{x2, y1}, {quad.u2, quad.v1}, quad.color});
        vertices.push_back({{x1, y2}, {quad.u1, quad.v2}, quad.color});
        vertices.push_back({{x2, y1}, {quad.u2, quad.v1}, quad.color});
        vertices.push_back({{x2, y2}, {quad.u2, quad.v2}, quad.color});
        vertices.push_back({{x1, y2}, {quad.u1, quad.v2}, quad.color});
    }
    
    if (!vertices.empty()) {
        void* data;
        vkMapMemory(device, uiTextVertexBufferMemory, 0, vertices.size() * sizeof(UITextVertex), 0, &data);
        memcpy(data, vertices.data(), vertices.size() * sizeof(UITextVertex));
        vkUnmapMemory(device, uiTextVertexBufferMemory);
        
        VkDeviceSize offsets = 0;
        vkCmdBindVertexBuffers(frames[currentFrame].cmdBuffer, 0, 1, &uiTextVertexBuffer, &offsets);
        vkCmdDraw(frames[currentFrame].cmdBuffer, (uint32_t)vertices.size(), 1, 0, 0);
    }
    
    uiTextQuads.clear();
}

void Vulkan::beginFrame() {
    if (!hwnd || !IsWindow(hwnd)) return;
    
    vkWaitForFences(device, 1, &frames[currentFrame].inFlightFence, VK_TRUE, UINT64_MAX);
    
    VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, 
        frames[currentFrame].imageAvailableSemaphore, VK_NULL_HANDLE, &currentImageIndex);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        recreateSwapchain();
        return;
    }
    
    vkResetFences(device, 1, &frames[currentFrame].inFlightFence);
    vkResetCommandBuffer(frames[currentFrame].cmdBuffer, 0);
    
    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(frames[currentFrame].cmdBuffer, &beginInfo);
    
    VkRenderPassBeginInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    rp.renderPass = renderPass;
    rp.framebuffer = framebuffers[currentImageIndex];
    rp.renderArea.offset = {0, 0};
    rp.renderArea.extent = swapchainExtent;
    
    VkClearValue clearValues[2];
    clearValues[0].color = {{0.1f, 0.1f, 0.2f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};
    
    rp.clearValueCount = 2;
    rp.pClearValues = clearValues;
    
    vkCmdBeginRenderPass(frames[currentFrame].cmdBuffer, &rp, VK_SUBPASS_CONTENTS_INLINE);
    
    updateUniformBuffer(currentFrame);
    renderModel();
    renderUI();
    renderUIText();
    renderUIImage();
    
    vkCmdEndRenderPass(frames[currentFrame].cmdBuffer);
    vkEndCommandBuffer(frames[currentFrame].cmdBuffer);
}

void Vulkan::endFrame() {
    if (!hwnd || !IsWindow(hwnd)) return;
    
    VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &frames[currentFrame].imageAvailableSemaphore;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &frames[currentFrame].cmdBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &frames[currentFrame].renderFinishedSemaphore;
    
    vkQueueSubmit(graphicsQueue, 1, &submitInfo, frames[currentFrame].inFlightFence);
}

void Vulkan::present() {
    if (!hwnd || !IsWindow(hwnd)) return;
    
    VkPresentInfoKHR presentInfo{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &frames[currentFrame].renderFinishedSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &currentImageIndex;
    
    VkResult result = vkQueuePresentKHR(graphicsQueue, &presentInfo);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        recreateSwapchain();
    }
    
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Vulkan::recreateSwapchain() {
    if (!hwnd || !IsWindow(hwnd)) return;
    
    RECT rect;
    GetClientRect(hwnd, &rect);
    int newWidth = rect.right - rect.left;
    int newHeight = rect.bottom - rect.top;
    
    if (newWidth <= 0 || newHeight <= 0) return;
    
    vkDeviceWaitIdle(device);
    
    cleanupSwapchain();
    vkDestroyImageView(device, depthImageView, nullptr);
    vkDestroyImage(device, depthImage, nullptr);
    vkFreeMemory(device, depthImageMemory, nullptr);
    
    width = newWidth;
    height = newHeight;
    
    createSwapchain();
    createFramebuffers();
}
VulkanTexture* Vulkan::loadUIImageFromData(unsigned char* data, int width, int height, int channels) {
    VulkanTexture* texture = new VulkanTexture();
    *texture = createTextureFromData(data, width, height, channels);
    if (texture->valid) {
        loadedUITextures.push_back(texture);
        return texture;
    }
    delete texture;
    return nullptr;
}