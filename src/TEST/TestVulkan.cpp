#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <array>
#include <string>
#include <cstring>
#include <map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "parser.h"
#include "stb_image.h"
#include "camera.h"
#include "Input.h"
#include "InterfaceManager.h"

#pragma comment(lib, "vulkan-1.lib")

const uint32_t WIDTH = 1280;
const uint32_t HEIGHT = 720;

struct VertexGPU { glm::vec3 pos; glm::vec3 color; glm::vec2 texCoord; };
struct UniformBufferObject { glm::mat4 model; glm::mat4 view; glm::mat4 proj; };

struct VulkanTexture {
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
    VkSampler sampler;
    int width, height;
};

// Глобальные переменные
Camera* camera = nullptr;
InterfaceManager* interfaceManager = nullptr;
float lastX = WIDTH / 2.0f;
float lastY = HEIGHT / 2.0f;
bool firstMouse = true;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) throw std::runtime_error("Failed to open " + filename);
    size_t fileSize = file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    return buffer;
}

uint32_t findMemoryType(VkPhysicalDevice physDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(physDevice, &memProps);
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    return 0;
}

VulkanTexture createTextureFromData(VkPhysicalDevice physDevice, VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, 
                                     unsigned char* data, int width, int height, int channels) {
    VulkanTexture tex;
    tex.width = width;
    tex.height = height;
    
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
    allocInfo.memoryTypeIndex = findMemoryType(physDevice, memReq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
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
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkCreateImage(device, &imageInfo, nullptr, &tex.image);
    
    vkGetImageMemoryRequirements(device, tex.image, &memReq);
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = findMemoryType(physDevice, memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkAllocateMemory(device, &allocInfo, nullptr, &tex.memory);
    vkBindImageMemory(device, tex.image, tex.memory, 0);
    
    VkCommandBufferAllocateInfo cmdAllocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cmdAllocInfo.commandPool = commandPool;
    cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAllocInfo.commandBufferCount = 1;
    VkCommandBuffer cmdBuffer;
    vkAllocateCommandBuffers(device, &cmdAllocInfo, &cmdBuffer);
    
    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmdBuffer, &beginInfo);
    
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
    vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    
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
    vkCmdCopyBufferToImage(cmdBuffer, stagingBuffer, tex.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    
    vkEndCommandBuffer(cmdBuffer);
    VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuffer;
    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);
    vkFreeCommandBuffers(device, commandPool, 1, &cmdBuffer);
    
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
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    vkCreateSampler(device, &samplerInfo, nullptr, &tex.sampler);
    
    return tex;
}

VulkanTexture createTextureFromEmbedded(VkPhysicalDevice physDevice, VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, const aiTexture* embeddedTexture) {
    int width, height, channels;
    unsigned char* data = nullptr;
    
    if (embeddedTexture->mHeight == 0) {
        data = stbi_load_from_memory(
            reinterpret_cast<const unsigned char*>(embeddedTexture->pcData),
            embeddedTexture->mWidth,
            &width, &height, &channels, 4
        );
    } else {
        width = embeddedTexture->mWidth;
        height = embeddedTexture->mHeight;
        channels = 4;
        data = new unsigned char[width * height * 4];
        for (unsigned int i = 0; i < width * height; i++) {
            data[i*4] = embeddedTexture->pcData[i].b;
            data[i*4+1] = embeddedTexture->pcData[i].g;
            data[i*4+2] = embeddedTexture->pcData[i].r;
            data[i*4+3] = embeddedTexture->pcData[i].a;
        }
    }
    
    if (!data) {
        unsigned char white[] = {255, 255, 255, 255};
        return createTextureFromData(physDevice, device, commandPool, graphicsQueue, white, 1, 1, 4);
    }
    
    VulkanTexture tex = createTextureFromData(physDevice, device, commandPool, graphicsQueue, data, width, height, channels);
    
    if (embeddedTexture->mHeight == 0) {
        stbi_image_free(data);
    } else {
        delete[] data;
    }
    
    return tex;
}

void compileShaders() {
    system("mkdir autoshadertest 2>nul");
    
    const char* vertCode = "#version 450\nlayout(binding = 0) uniform UniformBufferObject { mat4 model; mat4 view; mat4 proj; } ubo; layout(location = 0) in vec3 inPosition; layout(location = 1) in vec3 inColor; layout(location = 2) in vec2 inTexCoord; layout(location = 0) out vec2 fragTexCoord; void main() { gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0); fragTexCoord = inTexCoord; }";
    const char* fragCode = "#version 450\nlayout(binding = 1) uniform sampler2D texSampler; layout(location = 0) in vec2 fragTexCoord; layout(location = 0) out vec4 outColor; void main() { outColor = texture(texSampler, fragTexCoord); }";
    
    std::ofstream vertFile("autoshadertest/vert.vert"); vertFile << vertCode; vertFile.close();
    std::ofstream fragFile("autoshadertest/frag.frag"); fragFile << fragCode; fragFile.close();
    
    system("glslc autoshadertest/vert.vert -o autoshadertest/vert.spv");
    system("glslc autoshadertest/frag.frag -o autoshadertest/frag.spv");
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) PostQuitMessage(0);
            break;
        case WM_MOUSEMOVE:
            if (interfaceManager) {
                interfaceManager->handleMouseMove(LOWORD(lParam), HIWORD(lParam));
            }
            break;
        case WM_LBUTTONDOWN:
            if (interfaceManager) {
                interfaceManager->handleClick(LOWORD(lParam), HIWORD(lParam));
                interfaceManager->handleMouseDown(LOWORD(lParam), HIWORD(lParam));
            }
            break;
        case WM_LBUTTONUP:
            if (interfaceManager) {
                interfaceManager->handleMouseUp(LOWORD(lParam), HIWORD(lParam));
            }
            break;
        case WM_SIZE:
            if (interfaceManager) {
                interfaceManager->updateWindowSize(LOWORD(lParam), HIWORD(lParam));
            }
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    std::cout << "=== VULKAN MODEL VIEWER WITH UI ===" << std::endl;
    
    // ИНИЦИАЛИЗАЦИЯ КАМЕРЫ
    camera = new Camera(glm::vec3(0.0f, 50.0f, 200.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f);
    
    ModelParser parser;
    if (!parser.loadModel("101.fbx")) {
        std::cout << "ERROR: Failed to load 101.fbx!" << std::endl;
        system("pause");
        return -1;
    }
    
    const auto& meshes = parser.getMeshes();
    std::vector<VertexGPU> vertices;
    std::vector<uint32_t> indices;
    
    for (const auto& mesh : meshes) {
        for (const auto& vert : mesh.vertices) {
            VertexGPU v;
            v.pos = glm::vec3(vert.position[0], vert.position[1], vert.position[2]);
            v.color = glm::vec3(1.0f, 1.0f, 1.0f);
            v.texCoord = glm::vec2(vert.texCoords[0], vert.texCoords[1]);
            vertices.push_back(v);
        }
        for (unsigned int idx : mesh.indices) indices.push_back(idx);
    }
    
    if (vertices.empty()) {
        std::cout << "ERROR: No vertices!" << std::endl;
        system("pause");
        return -1;
    }
    
    std::cout << "Vertices: " << vertices.size() << ", Indices: " << indices.size() << std::endl;
    
    compileShaders();
    
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "VulkanWindow";
    RegisterClass(&wc);
    
    HWND hwnd = CreateWindowEx(0, "VulkanWindow", "Model Viewer with UI", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, WIDTH, HEIGHT, NULL, NULL, hInstance, NULL);
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    
    // VULKAN INIT
    VkInstance instance;
    VkApplicationInfo appInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO}; appInfo.apiVersion = VK_API_VERSION_1_0;
    const char* exts[] = {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME};
    VkInstanceCreateInfo instInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instInfo.pApplicationInfo = &appInfo;
    instInfo.enabledExtensionCount = 2;
    instInfo.ppEnabledExtensionNames = exts;
    vkCreateInstance(&instInfo, nullptr, &instance);
    
    VkSurfaceKHR surface;
    VkWin32SurfaceCreateInfoKHR surfInfo{VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
    surfInfo.hinstance = hInstance;
    surfInfo.hwnd = hwnd;
    vkCreateWin32SurfaceKHR(instance, &surfInfo, nullptr, &surface);
    
    uint32_t deviceCount;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
    VkPhysicalDevice physDevice = devices[0];
    
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
    
    VkDevice device;
    vkCreateDevice(physDevice, &devInfo, nullptr, &device);
    
    VkQueue graphicsQueue;
    vkGetDeviceQueue(device, queueFamily, 0, &graphicsQueue);
    
    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDevice, surface, &caps);
    VkSurfaceFormatKHR format{VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    VkExtent2D extent = caps.currentExtent;
    
    VkSwapchainKHR swapchain;
    VkSwapchainCreateInfoKHR swapInfo{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    swapInfo.surface = surface;
    swapInfo.minImageCount = 2;
    swapInfo.imageFormat = format.format;
    swapInfo.imageColorSpace = format.colorSpace;
    swapInfo.imageExtent = extent;
    swapInfo.imageArrayLayers = 1;
    swapInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapInfo.preTransform = caps.currentTransform;
    swapInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapInfo.clipped = VK_TRUE;
    vkCreateSwapchainKHR(device, &swapInfo, nullptr, &swapchain);
    
    uint32_t imageCount;
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
    std::vector<VkImage> swapchainImages(imageCount);
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data());
    
    std::vector<VkImageView> swapchainImageViews(imageCount);
    for (uint32_t i = 0; i < imageCount; i++) {
        VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        viewInfo.image = swapchainImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format.format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;
        vkCreateImageView(device, &viewInfo, nullptr, &swapchainImageViews[i]);
    }
    
    VkCommandPool commandPool;
    VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool);
    
    VkCommandBuffer cmdBuffer;
    VkCommandBufferAllocateInfo allocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    vkAllocateCommandBuffers(device, &allocInfo, &cmdBuffer);
    
    VkSemaphore imageAvailableSem, renderFinishedSem;
    VkSemaphoreCreateInfo semInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    vkCreateSemaphore(device, &semInfo, nullptr, &imageAvailableSem);
    vkCreateSemaphore(device, &semInfo, nullptr, &renderFinishedSem);
    
    // VERTEX BUFFER
    VkBuffer vertexBuffer, indexBuffer;
    VkDeviceMemory vertexBufferMemory, indexBufferMemory;
    VkDeviceSize vertSize = sizeof(vertices[0]) * vertices.size();
    VkBufferCreateInfo bufInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufInfo.size = vertSize;
    bufInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    vkCreateBuffer(device, &bufInfo, nullptr, &vertexBuffer);
    
    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(device, vertexBuffer, &memReq);
    VkMemoryAllocateInfo memAlloc{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    memAlloc.allocationSize = memReq.size;
    memAlloc.memoryTypeIndex = findMemoryType(physDevice, memReq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkAllocateMemory(device, &memAlloc, nullptr, &vertexBufferMemory);
    vkBindBufferMemory(device, vertexBuffer, vertexBufferMemory, 0);
    
    void* data;
    vkMapMemory(device, vertexBufferMemory, 0, vertSize, 0, &data);
    memcpy(data, vertices.data(), vertSize);
    vkUnmapMemory(device, vertexBufferMemory);
    
    VkDeviceSize idxSize = sizeof(indices[0]) * indices.size();
    bufInfo.size = idxSize;
    bufInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    vkCreateBuffer(device, &bufInfo, nullptr, &indexBuffer);
    vkGetBufferMemoryRequirements(device, indexBuffer, &memReq);
    memAlloc.allocationSize = memReq.size;
    memAlloc.memoryTypeIndex = findMemoryType(physDevice, memReq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkAllocateMemory(device, &memAlloc, nullptr, &indexBufferMemory);
    vkBindBufferMemory(device, indexBuffer, indexBufferMemory, 0);
    vkMapMemory(device, indexBufferMemory, 0, idxSize, 0, &data);
    memcpy(data, indices.data(), idxSize);
    vkUnmapMemory(device, indexBufferMemory);
    
    // UNIFORM BUFFER
    VkBuffer uniformBuffer;
    VkDeviceMemory uniformBufferMemory;
    VkDeviceSize uboSize = sizeof(UniformBufferObject);
    bufInfo.size = uboSize;
    bufInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    vkCreateBuffer(device, &bufInfo, nullptr, &uniformBuffer);
    vkGetBufferMemoryRequirements(device, uniformBuffer, &memReq);
    memAlloc.allocationSize = memReq.size;
    memAlloc.memoryTypeIndex = findMemoryType(physDevice, memReq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkAllocateMemory(device, &memAlloc, nullptr, &uniformBufferMemory);
    vkBindBufferMemory(device, uniformBuffer, uniformBufferMemory, 0);
    
    // ЗАГРУЗКА ТЕКСТУР
    std::vector<VulkanTexture> meshTextures;
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile("101.fbx", aiProcess_Triangulate | aiProcess_FlipUVs);
    
    for (size_t i = 0; i < meshes.size(); i++) {
        if (scene && scene->mNumTextures > 0 && !meshes[i].textures.empty()) {
            bool found = false;
            for (unsigned int t = 0; t < scene->mNumTextures; t++) {
                if (scene->mTextures[t]) {
                    std::string texName = scene->mTextures[t]->mFilename.C_Str();
                    if (texName.find("Diffuse") != std::string::npos) {
                        VulkanTexture tex = createTextureFromEmbedded(physDevice, device, commandPool, graphicsQueue, scene->mTextures[t]);
                        meshTextures.push_back(tex);
                        found = true;
                        break;
                    }
                }
            }
            if (!found) {
                unsigned char white[] = {255, 255, 255, 255};
                VulkanTexture tex = createTextureFromData(physDevice, device, commandPool, graphicsQueue, white, 1, 1, 4);
                meshTextures.push_back(tex);
            }
        } else {
            unsigned char white[] = {255, 255, 255, 255};
            VulkanTexture tex = createTextureFromData(physDevice, device, commandPool, graphicsQueue, white, 1, 1, 4);
            meshTextures.push_back(tex);
        }
    }
    
    // DESCRIPTOR SET LAYOUT
    VkDescriptorSetLayoutBinding uboBinding{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr};
    VkDescriptorSetLayoutBinding samplerBinding{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboBinding, samplerBinding};
    
    VkDescriptorSetLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layoutInfo.bindingCount = bindings.size();
    layoutInfo.pBindings = bindings.data();
    VkDescriptorSetLayout descLayout;
    vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descLayout);
    
    VkDescriptorPoolSize poolSizes[] = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, (uint32_t)meshTextures.size()}
    };
    VkDescriptorPoolCreateInfo poolInfo2{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    poolInfo2.poolSizeCount = 2;
    poolInfo2.pPoolSizes = poolSizes;
    poolInfo2.maxSets = (uint32_t)meshTextures.size();
    VkDescriptorPool descPool;
    vkCreateDescriptorPool(device, &poolInfo2, nullptr, &descPool);
    
    // DESCRIPTOR SETS
    std::vector<VkDescriptorSet> descSets(meshTextures.size());
    for (size_t i = 0; i < meshTextures.size(); i++) {
        VkDescriptorSetAllocateInfo descAlloc{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        descAlloc.descriptorPool = descPool;
        descAlloc.descriptorSetCount = 1;
        descAlloc.pSetLayouts = &descLayout;
        vkAllocateDescriptorSets(device, &descAlloc, &descSets[i]);
        
        VkDescriptorBufferInfo bufInfo2{uniformBuffer, 0, uboSize};
        VkDescriptorImageInfo imageInfo{meshTextures[i].sampler, meshTextures[i].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        
        std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};
        descriptorWrites[0] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        descriptorWrites[0].dstSet = descSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].pBufferInfo = &bufInfo2;
        
        descriptorWrites[1] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        descriptorWrites[1].dstSet = descSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].pImageInfo = &imageInfo;
        
        vkUpdateDescriptorSets(device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
    }
    
    // RENDER PASS
    VkAttachmentDescription colorAtt{};
    colorAtt.format = format.format;
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
    VkRenderPass renderPass;
    vkCreateRenderPass(device, &rpInfo, nullptr, &renderPass);
    
    // PIPELINE
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
    
    VkViewport vp{0,0,(float)WIDTH,(float)HEIGHT,0,1};
    VkRect2D scissor{{0,0},{WIDTH,HEIGHT}};
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
    
    VkPipelineLayoutCreateInfo plInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    plInfo.setLayoutCount = 1;
    plInfo.pSetLayouts = &descLayout;
    VkPipelineLayout pipelineLayout;
    vkCreatePipelineLayout(device, &plInfo, nullptr, &pipelineLayout);
    
    auto vertSpv = readFile("autoshadertest/vert.spv");
    auto fragSpv = readFile("autoshadertest/frag.spv");
    
    VkShaderModuleCreateInfo vertModInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    vertModInfo.codeSize = vertSpv.size();
    vertModInfo.pCode = (uint32_t*)vertSpv.data();
    VkShaderModule vertModule;
    vkCreateShaderModule(device, &vertModInfo, nullptr, &vertModule);
    
    VkShaderModuleCreateInfo fragModInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    fragModInfo.codeSize = fragSpv.size();
    fragModInfo.pCode = (uint32_t*)fragSpv.data();
    VkShaderModule fragModule;
    vkCreateShaderModule(device, &fragModInfo, nullptr, &fragModule);
    
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
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    
    VkPipeline graphicsPipeline;
    vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline);
    
    // FRAMEBUFFERS
    std::vector<VkFramebuffer> framebuffers(imageCount);
    for (uint32_t i = 0; i < imageCount; i++) {
        VkFramebufferCreateInfo fbInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        fbInfo.renderPass = renderPass;
        fbInfo.attachmentCount = 1;
        fbInfo.pAttachments = &swapchainImageViews[i];
        fbInfo.width = WIDTH;
        fbInfo.height = HEIGHT;
        fbInfo.layers = 1;
        vkCreateFramebuffer(device, &fbInfo, nullptr, &framebuffers[i]);
    }
    
    // ИНИЦИАЛИЗАЦИЯ UI
    interfaceManager = new InterfaceManager(nullptr, RenderAPI::VULKAN);
    interfaceManager->setWindow(nullptr);
    interfaceManager->initializeRender(hwnd, WIDTH, HEIGHT);
    
    std::cout << "\n=== CONTROLS ===" << std::endl;
    std::cout << "WASD - Move camera" << std::endl;
    std::cout << "Mouse (hold right click) - Look around" << std::endl;
    std::cout << "ESC - Exit" << std::endl;
    std::cout << "UI panels are rendered on top of 3D scene" << std::endl;
    
    // ЗАХВАТ МЫШИ
    ShowCursor(TRUE);
    RECT rect;
    GetClientRect(hwnd, &rect);
    POINT center;
    center.x = (rect.left + rect.right) / 2;
    center.y = (rect.top + rect.bottom) / 2;
    ClientToScreen(hwnd, &center);
    
    MSG msg = {};
    size_t currVertexOffset = 0;
    size_t currIndexOffset = 0;
    std::vector<size_t> meshVertexOffsets(meshes.size());
    std::vector<size_t> meshIndexOffsets(meshes.size());
    
    for (size_t i = 0; i < meshes.size(); i++) {
        meshVertexOffsets[i] = currVertexOffset;
        meshIndexOffsets[i] = currIndexOffset;
        currVertexOffset += meshes[i].vertices.size();
        currIndexOffset += meshes[i].indices.size();
    }
    
    LARGE_INTEGER frequency, lastTime, currentTime;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&lastTime);
    bool mouseCaptured = false;
    
    while (true) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) break;
            if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) break;
        }
        if (msg.message == WM_QUIT || (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE)) break;
        
        // Delta time
        QueryPerformanceCounter(&currentTime);
        deltaTime = (float)(currentTime.QuadPart - lastTime.QuadPart) / frequency.QuadPart;
        if (deltaTime > 0.1f) deltaTime = 0.1f;
        lastTime = currentTime;
        
        // ЗАХВАТ МЫШИ ПО ПРАВОЙ КНОПКЕ
        if (GetAsyncKeyState(VK_RBUTTON) & 0x8000) {
            if (!mouseCaptured) {
                mouseCaptured = true;
                ShowCursor(FALSE);
                GetClientRect(hwnd, &rect);
                center.x = (rect.left + rect.right) / 2;
                center.y = (rect.top + rect.bottom) / 2;
                ClientToScreen(hwnd, &center);
                SetCursorPos(center.x, center.y);
            }
            
            POINT mousePos;
            GetCursorPos(&mousePos);
            int dx = mousePos.x - center.x;
            int dy = mousePos.y - center.y;
            
            if (dx != 0 || dy != 0) {
                float sensitivity = 0.2f;
                camera->ProcessMouseMovement((float)dx * sensitivity, (float)dy * sensitivity, true);
                SetCursorPos(center.x, center.y);
            }
        } else {
            if (mouseCaptured) {
                mouseCaptured = false;
                ShowCursor(TRUE);
            }
        }
        
        // ДВИЖЕНИЕ КАМЕРЫ
        float speed = 20.0f;
        glm::vec3 forward = camera->GetFront();
        glm::vec3 right = camera->GetRight();
        glm::vec3 up = camera->GetUp();
        
        if (GetAsyncKeyState('W') & 0x8000) camera->SetPosition(camera->GetPosition() + forward * speed * deltaTime);
        if (GetAsyncKeyState('S') & 0x8000) camera->SetPosition(camera->GetPosition() - forward * speed * deltaTime);
        if (GetAsyncKeyState('A') & 0x8000) camera->SetPosition(camera->GetPosition() - right * speed * deltaTime);
        if (GetAsyncKeyState('D') & 0x8000) camera->SetPosition(camera->GetPosition() + right * speed * deltaTime);
        if (GetAsyncKeyState('Q') & 0x8000) camera->SetPosition(camera->GetPosition() + up * speed * deltaTime);
        if (GetAsyncKeyState('E') & 0x8000) camera->SetPosition(camera->GetPosition() - up * speed * deltaTime);
        
        // UNIFORM BUFFER
        UniformBufferObject ubo;
        ubo.model = glm::mat4(1.0f);
        ubo.view = camera->GetViewMatrix();
        ubo.proj = glm::perspective(glm::radians(45.0f), (float)WIDTH/HEIGHT, 0.1f, 1000.0f);
        ubo.proj[1][1] *= -1;
        
        void* uboData;
        vkMapMemory(device, uniformBufferMemory, 0, sizeof(ubo), 0, &uboData);
        memcpy(uboData, &ubo, sizeof(ubo));
        vkUnmapMemory(device, uniformBufferMemory);
        
        uint32_t imageIndex;
        vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailableSem, VK_NULL_HANDLE, &imageIndex);
        
        vkResetCommandBuffer(cmdBuffer, 0);
        VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        vkBeginCommandBuffer(cmdBuffer, &beginInfo);
        
        VkRenderPassBeginInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        rp.renderPass = renderPass;
        rp.framebuffer = framebuffers[imageIndex];
        rp.renderArea.extent = {WIDTH, HEIGHT};
        VkClearValue clear = {{{0.1f, 0.1f, 0.2f, 1.0f}}};
        rp.clearValueCount = 1;
        rp.pClearValues = &clear;
        vkCmdBeginRenderPass(cmdBuffer, &rp, VK_SUBPASS_CONTENTS_INLINE);
        
        vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
        VkDeviceSize offsets = 0;
        vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffer, &offsets);
        vkCmdBindIndexBuffer(cmdBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        
        for (size_t i = 0; i < meshes.size(); i++) {
            if (meshes[i].indices.size() > 0) {
                vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descSets[i], 0, nullptr);
                vkCmdDrawIndexed(cmdBuffer, meshes[i].indices.size(), 1, meshIndexOffsets[i], meshVertexOffsets[i], 0);
            }
        }
        
        vkCmdEndRenderPass(cmdBuffer);
        
        // РЕНДЕР UI ПОВЕРХ 3D СЦЕНЫ
        if (interfaceManager) {
            interfaceManager->renderStatic();
        }
        
        vkEndCommandBuffer(cmdBuffer);
        
        VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &imageAvailableSem;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmdBuffer;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &renderFinishedSem;
        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        
        VkPresentInfoKHR presentInfo{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &renderFinishedSem;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchain;
        presentInfo.pImageIndices = &imageIndex;
        vkQueuePresentKHR(graphicsQueue, &presentInfo);
        
        vkQueueWaitIdle(graphicsQueue);
    }
    
    vkDeviceWaitIdle(device);
    for (auto fb : framebuffers) vkDestroyFramebuffer(device, fb, nullptr);
    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);
    vkDestroyDescriptorPool(device, descPool, nullptr);
    vkDestroyDescriptorSetLayout(device, descLayout, nullptr);
    vkDestroyBuffer(device, vertexBuffer, nullptr);
    vkFreeMemory(device, vertexBufferMemory, nullptr);
    vkDestroyBuffer(device, indexBuffer, nullptr);
    vkFreeMemory(device, indexBufferMemory, nullptr);
    vkDestroyBuffer(device, uniformBuffer, nullptr);
    vkFreeMemory(device, uniformBufferMemory, nullptr);
    for (auto& tex : meshTextures) {
        vkDestroyImageView(device, tex.view, nullptr);
        vkDestroyImage(device, tex.image, nullptr);
        vkFreeMemory(device, tex.memory, nullptr);
        vkDestroySampler(device, tex.sampler, nullptr);
    }
    vkDestroyShaderModule(device, vertModule, nullptr);
    vkDestroyShaderModule(device, fragModule, nullptr);
    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroySemaphore(device, imageAvailableSem, nullptr);
    vkDestroySemaphore(device, renderFinishedSem, nullptr);
    for (auto iv : swapchainImageViews) vkDestroyImageView(device, iv, nullptr);
    vkDestroySwapchainKHR(device, swapchain, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
    
    delete interfaceManager;
    delete camera;
    ShowCursor(TRUE);
    FreeConsole();
    return 0;
}