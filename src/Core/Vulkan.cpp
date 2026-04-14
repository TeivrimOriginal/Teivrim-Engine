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
    
    std::ofstream vertFile("autoshadertest/vert.vert"); vertFile << vertCode; vertFile.close();
    std::ofstream fragFile("autoshadertest/frag.frag"); fragFile << fragCode; fragFile.close();
    std::ofstream uiVertFile("autoshadertest/ui_vert.vert"); uiVertFile << uiVertCode; uiVertFile.close();
    std::ofstream uiFragFile("autoshadertest/ui_frag.frag"); uiFragFile << uiFragCode; uiFragFile.close();
    
    system("glslc autoshadertest/vert.vert -o autoshadertest/vert.spv");
    system("glslc autoshadertest/frag.frag -o autoshadertest/frag.spv");
    system("glslc autoshadertest/ui_vert.vert -o autoshadertest/ui_vert.spv");
    system("glslc autoshadertest/ui_frag.frag -o autoshadertest/ui_frag.spv");
}

Vulkan::Vulkan(HWND hwnd, int width, int height) 
    : hwnd(hwnd), width(width), height(height), initialized(false), currentFrame(0), currentImageIndex(0), modelLoaded(false),
      instance(VK_NULL_HANDLE), physDevice(VK_NULL_HANDLE), device(VK_NULL_HANDLE),
      graphicsQueue(VK_NULL_HANDLE), surface(VK_NULL_HANDLE), swapchain(VK_NULL_HANDLE),
      renderPass(VK_NULL_HANDLE), pipelineLayout3D(VK_NULL_HANDLE), pipelineLayoutUI(VK_NULL_HANDLE),
      pipeline3D(VK_NULL_HANDLE), pipelineUI(VK_NULL_HANDLE), commandPool(VK_NULL_HANDLE),
      descLayout(VK_NULL_HANDLE), descPool(VK_NULL_HANDLE),
      vertexBuffer(VK_NULL_HANDLE), indexBuffer(VK_NULL_HANDLE),
      vertexBufferMemory(VK_NULL_HANDLE), indexBufferMemory(VK_NULL_HANDLE),
      uiVertexBuffer(VK_NULL_HANDLE), uiVertexBufferMemory(VK_NULL_HANDLE) {
    
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        frames[i].cmdBuffer = VK_NULL_HANDLE;
        frames[i].imageAvailableSemaphore = VK_NULL_HANDLE;
        frames[i].renderFinishedSemaphore = VK_NULL_HANDLE;
        frames[i].inFlightFence = VK_NULL_HANDLE;
        frames[i].uniformBuffer = VK_NULL_HANDLE;
        frames[i].uniformBufferMemory = VK_NULL_HANDLE;
    }
    
    compileShaders();
    
    // Instance
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
    
    // Surface
    VkWin32SurfaceCreateInfoKHR surfInfo{VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
    surfInfo.hinstance = GetModuleHandle(NULL);
    surfInfo.hwnd = hwnd;
    if (vkCreateWin32SurfaceKHR(instance, &surfInfo, nullptr, &surface) != VK_SUCCESS) {
        std::cerr << "Failed to create surface" << std::endl;
        return;
    }
    
    // Physical device
    uint32_t deviceCount;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        std::cerr << "No Vulkan devices found" << std::endl;
        return;
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
    physDevice = devices[0];
    
    // Device
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
    
    // Create all Vulkan objects
    createSwapchain();
    createRenderPass();
    createFramebuffers();
    createCommandPool();
    createUniformBuffers();
    createSyncObjects();
    createDescriptorSetLayout();
    createPipelines();
    createUIBuffers();
    
    // Allocate command buffers (one per frame)
    VkCommandBufferAllocateInfo allocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;
    
    std::vector<VkCommandBuffer> cmdBuffers(MAX_FRAMES_IN_FLIGHT);
    vkAllocateCommandBuffers(device, &allocInfo, cmdBuffers.data());
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        frames[i].cmdBuffer = cmdBuffers[i];
    }
    
    viewMat = glm::lookAt(glm::vec3(0.0f, 50.0f, 150.0f), glm::vec3(0, 50, 0), glm::vec3(0, 1, 0));
    projMat = glm::perspective(glm::radians(45.0f), (float)width/height, 0.1f, 1000.0f);
    projMat[1][1] *= -1;
    modelMat = glm::mat4(1.0f);
    
    initialized = true;
    std::cout << "Vulkan initialized successfully (AAA mode: " << MAX_FRAMES_IN_FLIGHT << " frames in flight)" << std::endl;
}

Vulkan::~Vulkan() {
    if (device) {
        vkDeviceWaitIdle(device);
        
        cleanupFrameResources();
        
        vkDestroyCommandPool(device, commandPool, nullptr);
        vkDestroyPipeline(device, pipeline3D, nullptr);
        vkDestroyPipeline(device, pipelineUI, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout3D, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayoutUI, nullptr);
        vkDestroyRenderPass(device, renderPass, nullptr);
        vkDestroyDescriptorPool(device, descPool, nullptr);
        vkDestroyDescriptorSetLayout(device, descLayout, nullptr);
        vkDestroyBuffer(device, vertexBuffer, nullptr);
        vkFreeMemory(device, vertexBufferMemory, nullptr);
        vkDestroyBuffer(device, indexBuffer, nullptr);
        vkFreeMemory(device, indexBufferMemory, nullptr);
        vkDestroyBuffer(device, uiVertexBuffer, nullptr);
        vkFreeMemory(device, uiVertexBufferMemory, nullptr);
        
        cleanupTextures();
        
        for (auto fb : framebuffers) vkDestroyFramebuffer(device, fb, nullptr);
        for (auto iv : swapchainImageViews) vkDestroyImageView(device, iv, nullptr);
        vkDestroySwapchainKHR(device, swapchain, nullptr);
        vkDestroyDevice(device, nullptr);
    }
    if (surface) vkDestroySurfaceKHR(instance, surface, nullptr);
    if (instance) vkDestroyInstance(instance, nullptr);
}

void Vulkan::cleanupFrameResources() {
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (frames[i].uniformBuffer) vkDestroyBuffer(device, frames[i].uniformBuffer, nullptr);
        if (frames[i].uniformBufferMemory) vkFreeMemory(device, frames[i].uniformBufferMemory, nullptr);
        if (frames[i].renderFinishedSemaphore) vkDestroySemaphore(device, frames[i].renderFinishedSemaphore, nullptr);
        if (frames[i].imageAvailableSemaphore) vkDestroySemaphore(device, frames[i].imageAvailableSemaphore, nullptr);
        if (frames[i].inFlightFence) vkDestroyFence(device, frames[i].inFlightFence, nullptr);
    }
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
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
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
    
    // Copy buffer to image via command buffer
    VkCommandBufferAllocateInfo cmdAllocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cmdAllocInfo.commandPool = commandPool;
    cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAllocInfo.commandBufferCount = 1;
    VkCommandBuffer copyCmd;
    vkAllocateCommandBuffers(device, &cmdAllocInfo, &copyCmd);
    
    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(copyCmd, &beginInfo);
    
    VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
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
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
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
    vkFreeCommandBuffers(device, commandPool, 1, &copyCmd);
    
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
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
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

void Vulkan::loadModel(const std::vector<StandardMesh>& meshes) {
    modelVertices.clear();
    modelIndices.clear();
    meshVertexOffsets.clear();
    meshIndexOffsets.clear();
    
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
            modelIndices.push_back(currVertexOffset + idx);
        }
        
        currVertexOffset += mesh.vertices.size();
        currIndexOffset += mesh.indices.size();
    }
    
    if (modelVertices.empty()) return;
    
    cleanupTextures();
    for (size_t i = 0; i < meshes.size(); i++) {
        meshTextures.push_back(createWhiteTexture());
    }
    
    createModelBuffers();
    createDescriptorPoolAndSets();
    
    modelLoaded = true;
    std::cout << "Model loaded: " << modelVertices.size() << " vertices, " << modelIndices.size() << " indices" << std::endl;
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

void Vulkan::createUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
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
    layoutInfo.bindingCount = bindings.size();
    layoutInfo.pBindings = bindings.data();
    vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descLayout);
}

void Vulkan::createDescriptorPoolAndSets() {
    if (descPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, descPool, nullptr);
    }
    
    uint32_t textureCount = std::max(1u, (uint32_t)meshTextures.size());
    
    VkDescriptorPoolSize poolSizes[] = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, textureCount * MAX_FRAMES_IN_FLIGHT},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, textureCount}
    };
    
    VkDescriptorPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    poolInfo.poolSizeCount = 2;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = textureCount * MAX_FRAMES_IN_FLIGHT;
    vkCreateDescriptorPool(device, &poolInfo, nullptr, &descPool);
    
    descSets.clear();
    descSets.resize(textureCount);
    
    for (uint32_t i = 0; i < textureCount; i++) {
        VkDescriptorSetAllocateInfo descAlloc{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        descAlloc.descriptorPool = descPool;
        descAlloc.descriptorSetCount = 1;
        descAlloc.pSetLayouts = &descLayout;
        vkAllocateDescriptorSets(device, &descAlloc, &descSets[i]);
        
        VulkanTexture& tex = (i < meshTextures.size() && meshTextures[i].valid) ? meshTextures[i] : meshTextures[0];
        VkDescriptorImageInfo imageInfo{tex.sampler, tex.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        
        // Use uniform buffer for current frame
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
        
        vkUpdateDescriptorSets(device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
    }
}

void Vulkan::createSwapchain() {
    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDevice, surface, &caps);
    VkSurfaceFormatKHR format{VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    swapchainFormat = format.format;
    
    RECT rect;
    GetClientRect(hwnd, &rect);
    int clientWidth = rect.right - rect.left;
    int clientHeight = rect.bottom - rect.top;
    if (clientWidth <= 0) clientWidth = width;
    if (clientHeight <= 0) clientHeight = height;
    
    if (caps.currentExtent.width != 0 && caps.currentExtent.width != 0xFFFFFFFF) {
        swapchainExtent = caps.currentExtent;
    } else {
        swapchainExtent.width = std::max(caps.minImageExtent.width, std::min(caps.maxImageExtent.width, (uint32_t)clientWidth));
        swapchainExtent.height = std::max(caps.minImageExtent.height, std::min(caps.maxImageExtent.height, (uint32_t)clientHeight));
    }
    
    VkSwapchainCreateInfoKHR swapInfo{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    swapInfo.surface = surface;
    swapInfo.minImageCount = 2;
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
    
    swapchainImageViews.resize(imageCount);
    for (uint32_t i = 0; i < imageCount; i++) {
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
    
    VkRenderPassCreateInfo rpInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    rpInfo.attachmentCount = 1;
    rpInfo.pAttachments = &colorAtt;
    rpInfo.subpassCount = 1;
    rpInfo.pSubpasses = &subpass;
    vkCreateRenderPass(device, &rpInfo, nullptr, &renderPass);
}

void Vulkan::createFramebuffers() {
    framebuffers.resize(swapchainImageViews.size());
    for (size_t i = 0; i < swapchainImageViews.size(); i++) {
        VkFramebufferCreateInfo fbInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        fbInfo.renderPass = renderPass;
        fbInfo.attachmentCount = 1;
        fbInfo.pAttachments = &swapchainImageViews[i];
        fbInfo.width = swapchainExtent.width;
        fbInfo.height = swapchainExtent.height;
        fbInfo.layers = 1;
        vkCreateFramebuffer(device, &fbInfo, nullptr, &framebuffers[i]);
    }
}

void Vulkan::createCommandPool() {
    VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool);
}

void Vulkan::createSyncObjects() {
    VkSemaphoreCreateInfo semInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
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
    
    auto bindingDesc = VkVertexInputBindingDescription{0, sizeof(VertexGPU), VK_VERTEX_INPUT_RATE_VERTEX};
    auto attrDesc = std::array<VkVertexInputAttributeDescription, 3>{
        VkVertexInputAttributeDescription{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexGPU, pos)},
        VkVertexInputAttributeDescription{1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexGPU, color)},
        VkVertexInputAttributeDescription{2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexGPU, texCoord)}
    };
    
    VkPipelineVertexInputStateCreateInfo vi{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vi.vertexBindingDescriptionCount = 1;
    vi.pVertexBindingDescriptions = &bindingDesc;
    vi.vertexAttributeDescriptionCount = 3;
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
    raster.lineWidth = 1.0f;
    
    VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    
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
    uiVi.vertexAttributeDescriptionCount = 2;
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
    
    VkPipelineLayoutCreateInfo plInfoUI{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
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
    uiPipelineInfo.pColorBlendState = &uiCb;
    uiPipelineInfo.layout = pipelineLayoutUI;
    uiPipelineInfo.renderPass = renderPass;
    uiPipelineInfo.subpass = 0;
    vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &uiPipelineInfo, nullptr, &pipelineUI);
    
    vkDestroyShaderModule(device, vertModule, nullptr);
    vkDestroyShaderModule(device, fragModule, nullptr);
    vkDestroyShaderModule(device, uiVertModule, nullptr);
    vkDestroyShaderModule(device, uiFragModule, nullptr);
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

void Vulkan::updateUniformBuffer(int frameIndex) {
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
}

void Vulkan::drawQuad(float x1, float y1, float x2, float y2, float r, float g, float b) {
    UIQuad quad;
    quad.x1 = x1; quad.y1 = y1;
    quad.x2 = x2; quad.y2 = y2;
    quad.color = glm::vec3(r, g, b);
    uiQuads.push_back(quad);
}

void Vulkan::drawText(int x, int y, const std::string& text, float r, float g, float b) {}
void Vulkan::drawTextCentered(int x, int y, int w, int h, const std::string& text, float r, float g, float b) {}

void Vulkan::renderModel() {
    if (!modelLoaded || modelVertices.empty()) return;
    
    vkCmdBindPipeline(frames[currentFrame].cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline3D);
    VkDeviceSize offsets = 0;
    vkCmdBindVertexBuffers(frames[currentFrame].cmdBuffer, 0, 1, &vertexBuffer, &offsets);
    vkCmdBindIndexBuffer(frames[currentFrame].cmdBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    
    for (size_t i = 0; i < descSets.size() && i < meshTextures.size(); i++) {
        uint32_t indexCount = 0;
        if (i < meshIndexOffsets.size()) {
            uint32_t nextOffset = (i + 1 < meshIndexOffsets.size()) ? meshIndexOffsets[i + 1] : modelIndices.size();
            indexCount = nextOffset - meshIndexOffsets[i];
        }
        
        if (indexCount > 0 && i < descSets.size()) {
            vkCmdBindDescriptorSets(frames[currentFrame].cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout3D, 0, 1, &descSets[i], 0, nullptr);
            vkCmdDrawIndexed(frames[currentFrame].cmdBuffer, indexCount, 1, meshIndexOffsets[i], meshVertexOffsets[i], 0);
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
        vkCmdDraw(frames[currentFrame].cmdBuffer, vertices.size(), 1, 0, 0);
    }
    
    uiQuads.clear();
}

void Vulkan::beginFrame() {
    if (!hwnd || !IsWindow(hwnd)) {
        return;
    }
    
    vkWaitForFences(device, 1, &frames[currentFrame].inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &frames[currentFrame].inFlightFence);
    
    VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, 
        frames[currentFrame].imageAvailableSemaphore, VK_NULL_HANDLE, &currentImageIndex);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        recreateSwapchain();
        return;
    }
    
    vkResetCommandBuffer(frames[currentFrame].cmdBuffer, 0);
    
    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(frames[currentFrame].cmdBuffer, &beginInfo);
    
    VkRenderPassBeginInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    rp.renderPass = renderPass;
    rp.framebuffer = framebuffers[currentImageIndex];
    rp.renderArea.offset = {0, 0};
    rp.renderArea.extent = swapchainExtent;
    VkClearValue clear = {{{0.1f, 0.1f, 0.2f, 1.0f}}};
    rp.clearValueCount = 1;
    rp.pClearValues = &clear;
    
    vkCmdBeginRenderPass(frames[currentFrame].cmdBuffer, &rp, VK_SUBPASS_CONTENTS_INLINE);
}

void Vulkan::endFrame() {
    if (!hwnd || !IsWindow(hwnd)) {
        return;
    }
    
    updateUniformBuffer(currentFrame);
    renderModel();
    renderUI();
    
    vkCmdEndRenderPass(frames[currentFrame].cmdBuffer);
    vkEndCommandBuffer(frames[currentFrame].cmdBuffer);
    
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
    if (!hwnd || !IsWindow(hwnd)) {
        return;
    }
    
    VkPresentInfoKHR presentInfo{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &frames[currentFrame].renderFinishedSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &currentImageIndex;
    
    vkQueuePresentKHR(graphicsQueue, &presentInfo);
    
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Vulkan::recreateSwapchain() {
    if (!hwnd || !IsWindow(hwnd)) {
        std::cout << "[Vulkan] Window destroyed, skipping swapchain recreation" << std::endl;
        return;
    }
    
    RECT rect;
    GetClientRect(hwnd, &rect);
    int newWidth = rect.right - rect.left;
    int newHeight = rect.bottom - rect.top;
    
    if (newWidth <= 0 || newHeight <= 0) {
        std::cout << "[Vulkan] Invalid window size (" << newWidth << "x" << newHeight << "), skipping swapchain recreation" << std::endl;
        return;
    }
    
    vkDeviceWaitIdle(device);
    
    for (auto fb : framebuffers) vkDestroyFramebuffer(device, fb, nullptr);
    for (auto iv : swapchainImageViews) vkDestroyImageView(device, iv, nullptr);
    vkDestroySwapchainKHR(device, swapchain, nullptr);
    
    width = newWidth;
    height = newHeight;
    
    createSwapchain();
    createFramebuffers();
    
    vkDestroyPipeline(device, pipeline3D, nullptr);
    vkDestroyPipeline(device, pipelineUI, nullptr);
    createPipelines();
}