// SecondRender.cpp
#include "SecondRender.h"
#include "Vulkan.h"
#include "Render/Win32/RenderUI.h"
#include "../Interface/InterfaceManager.h"
#include "../Interface/Panels.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <float.h>

SecondRender::SecondRender() 
    : vulkan(nullptr)
    , renderUI(nullptr)
    , uiManager(nullptr)
    , screenW(1280)
    , screenH(720)
    , initialized(false)
    , backgroundEnabled(true)
    , overlayEnabled(true)
    , viewportX(0)
    , viewportY(0)
    , viewportW(0)
    , viewportH(0)
    , cameraMatrixValid(false)
    , needBufferUpdate(true)
    , needAxesUpdate(true)
    , lastCamX(0.0f)
    , lastCamZ(0.0f)
    , linePipeline(VK_NULL_HANDLE)
    , linePipelineLayout(VK_NULL_HANDLE)
    , lineVertexBuffer(VK_NULL_HANDLE)
    , axesVertexBuffer(VK_NULL_HANDLE)
    , lineVertexBufferMemory(VK_NULL_HANDLE)
    , axesVertexBufferMemory(VK_NULL_HANDLE)
    , lineVertexCount(0)
    , axesVertexCount(0)
{
    gridConfig.cellSize = 50;
    gridConfig.gridSize = 20;
    gridConfig.lineColor[0] = 0.4f;
    gridConfig.lineColor[1] = 0.4f;
    gridConfig.lineColor[2] = 0.45f;
    gridConfig.centerLineColor[0] = 0.8f;
    gridConfig.centerLineColor[1] = 0.8f;
    gridConfig.centerLineColor[2] = 1.0f;
    gridConfig.axisXColor[0] = 1.0f;
    gridConfig.axisXColor[1] = 0.0f;
    gridConfig.axisXColor[2] = 0.0f;
    gridConfig.axisZColor[0] = 0.0f;
    gridConfig.axisZColor[1] = 1.0f;
    gridConfig.axisZColor[2] = 0.0f;
    gridConfig.enabled = true;
    gridConfig.infiniteGrid = true;
    gridConfig.showAxes = true;
    gridConfig.gridSpacing = 20.0f;
    gridConfig.fadeDistance = 500.0f;
    gridConfig.yOffset = 0.0f;
    gridConfig.lineThickness = 1.0f;
    
    std::cout << "[SecondRender] Constructor called" << std::endl;
}

SecondRender::~SecondRender() {
    ClearBackground();
    ClearOverlay();
    DestroyLineResources();
    
    if (axesVertexBuffer != VK_NULL_HANDLE && vulkan) {
        vkDestroyBuffer(vulkan->getDevice(), axesVertexBuffer, nullptr);
        axesVertexBuffer = VK_NULL_HANDLE;
    }
    if (axesVertexBufferMemory != VK_NULL_HANDLE && vulkan) {
        vkFreeMemory(vulkan->getDevice(), axesVertexBufferMemory, nullptr);
        axesVertexBufferMemory = VK_NULL_HANDLE;
    }
}

void SecondRender::DestroyLineResources() {
    if (!vulkan) return;
    
    VkDevice device = vulkan->getDevice();
    if (device == VK_NULL_HANDLE) return;
    
    vkDeviceWaitIdle(device);
    
    if (linePipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, linePipeline, nullptr);
        linePipeline = VK_NULL_HANDLE;
    }
    if (linePipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, linePipelineLayout, nullptr);
        linePipelineLayout = VK_NULL_HANDLE;
    }
    if (lineVertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, lineVertexBuffer, nullptr);
        lineVertexBuffer = VK_NULL_HANDLE;
    }
    if (lineVertexBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, lineVertexBufferMemory, nullptr);
        lineVertexBufferMemory = VK_NULL_HANDLE;
    }
}

bool SecondRender::CreateLinePipeline() {
    if (!vulkan) return false;
    
    VkDevice device = vulkan->getDevice();
    if (device == VK_NULL_HANDLE) return false;
    
    VkShaderModule vertModule = vulkan->getLineVertShader();
    VkShaderModule fragModule = vulkan->getLineFragShader();
    
    if (vertModule == VK_NULL_HANDLE || fragModule == VK_NULL_HANDLE) {
        std::cerr << "[SecondRender] Line shaders not available" << std::endl;
        return false;
    }
    
    if (linePipeline != VK_NULL_HANDLE) return true;
    
    VkVertexInputBindingDescription bindingDesc{};
    bindingDesc.binding = 0;
    bindingDesc.stride = sizeof(LineVertex);
    bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    VkVertexInputAttributeDescription attrDesc[2] = {};
    attrDesc[0].location = 0;
    attrDesc[0].binding = 0;
    attrDesc[0].format = VK_FORMAT_R32G32_SFLOAT;
    attrDesc[0].offset = offsetof(LineVertex, pos);
    
    attrDesc[1].location = 1;
    attrDesc[1].binding = 0;
    attrDesc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrDesc[1].offset = offsetof(LineVertex, color);
    
    VkPipelineVertexInputStateCreateInfo vi{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vi.vertexBindingDescriptionCount = 1;
    vi.pVertexBindingDescriptions = &bindingDesc;
    vi.vertexAttributeDescriptionCount = 2;
    vi.pVertexAttributeDescriptions = attrDesc;
    
    VkPipelineInputAssemblyStateCreateInfo ia{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    ia.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    
    VkPipelineViewportStateCreateInfo vpState{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    vpState.viewportCount = 1;
    vpState.scissorCount = 1;
    
    VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.cullMode = VK_CULL_MODE_NONE;
    raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster.lineWidth = gridConfig.lineThickness;
    
    VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    
    VkPipelineDepthStencilStateCreateInfo depthStencil{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_FALSE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    
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
    
    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_LINE_WIDTH};
    VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamic.dynamicStateCount = 3;
    dynamic.pDynamicStates = dynamicStates;
    
    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushRange.offset = 0;
    pushRange.size = sizeof(glm::mat4);
    
    VkPipelineLayoutCreateInfo plInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    plInfo.pushConstantRangeCount = 1;
    plInfo.pPushConstantRanges = &pushRange;
    
    if (vkCreatePipelineLayout(device, &plInfo, nullptr, &linePipelineLayout) != VK_SUCCESS) {
        std::cerr << "[SecondRender] Failed to create line pipeline layout" << std::endl;
        return false;
    }
    
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
    pipelineInfo.pDynamicState = &dynamic;
    pipelineInfo.layout = linePipelineLayout;
    pipelineInfo.renderPass = vulkan->getRenderPass();
    pipelineInfo.subpass = 0;
    
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &linePipeline) != VK_SUCCESS) {
        std::cerr << "[SecondRender] Failed to create line pipeline" << std::endl;
        return false;
    }
    
    std::cout << "[SecondRender] Line pipeline created" << std::endl;
    return true;
}

void SecondRender::Initialize(Vulkan* vk, RenderUI* ui, InterfaceManager* manager, int screenWidth, int screenHeight) {
    vulkan = vk;
    renderUI = ui;
    uiManager = manager;
    screenW = screenWidth;
    screenH = screenHeight;
    initialized = (vulkan != nullptr);
    
    std::cout << "[SecondRender] Initialize called, vulkan=" << (vulkan ? "OK" : "NULL") << std::endl;
    
    UpdateViewportRect();
    
    if (initialized) {
        testQuadsDirty = true;
        CreateLinePipeline();
        needBufferUpdate = true;
        needAxesUpdate = true;
    }
}

void SecondRender::UpdateViewportRect() {
    if (!uiManager) {
        viewportX = 0;
        viewportY = 0;
        viewportW = screenW;
        viewportH = screenH;
        return;
    }
    
    Panel* view3D = uiManager->getPanelManager()->get3D();
    if (view3D && view3D->visible && !view3D->collapsed) {
        viewportX = view3D->getX();
        viewportY = view3D->getY();
        viewportW = view3D->getW();
        viewportH = view3D->getH();
        
        if (viewportX < 0) viewportX = 0;
        if (viewportY < 0) viewportY = 0;
        if (viewportX + viewportW > screenW) viewportW = screenW - viewportX;
        if (viewportY + viewportH > screenH) viewportH = screenH - viewportY;
    } else {
        viewportX = 0;
        viewportY = 0;
        viewportW = screenW;
        viewportH = screenH;
    }
    
    testQuadsDirty = true;
}

void SecondRender::DrawBackgroundQuad(float x1, float y1, float x2, float y2, float r, float g, float b) {
    if (!backgroundEnabled) return;
    
    Quad2D quad;
    quad.x1 = x1 + viewportX;
    quad.y1 = y1 + viewportY;
    quad.x2 = x2 + viewportX;
    quad.y2 = y2 + viewportY;
    quad.r = r; quad.g = g; quad.b = b;
    quad.useTexture = false;
    quad.textureId = nullptr;
    quad.layer = 0;
    quad.u1 = quad.v1 = 0.0f;
    quad.u2 = quad.v2 = 1.0f;
    
    backgroundQuads.push_back(quad);
}

void SecondRender::DrawOverlayQuad(float x1, float y1, float x2, float y2, float r, float g, float b) {
    if (!overlayEnabled) return;
    
    Quad2D quad;
    quad.x1 = x1 + viewportX;
    quad.y1 = y1 + viewportY;
    quad.x2 = x2 + viewportX;
    quad.y2 = y2 + viewportY;
    quad.r = r; quad.g = g; quad.b = b;
    quad.useTexture = false;
    quad.textureId = nullptr;
    quad.layer = 1;
    quad.u1 = quad.v1 = 0.0f;
    quad.u2 = quad.v2 = 1.0f;
    
    overlayQuads.push_back(quad);
}

void SecondRender::DrawBackgroundImage(float x1, float y1, float x2, float y2, void* texture) {
    if (!backgroundEnabled || !texture) return;
    
    Quad2D quad;
    quad.x1 = x1 + viewportX;
    quad.y1 = y1 + viewportY;
    quad.x2 = x2 + viewportX;
    quad.y2 = y2 + viewportY;
    quad.r = quad.g = quad.b = 1.0f;
    quad.useTexture = true;
    quad.textureId = texture;
    quad.layer = 0;
    quad.u1 = 0.0f; quad.v1 = 0.0f;
    quad.u2 = 1.0f; quad.v2 = 1.0f;
    
    backgroundQuads.push_back(quad);
}

void SecondRender::DrawOverlayImage(float x1, float y1, float x2, float y2, void* texture) {
    if (!overlayEnabled || !texture) return;
    
    Quad2D quad;
    quad.x1 = x1 + viewportX;
    quad.y1 = y1 + viewportY;
    quad.x2 = x2 + viewportX;
    quad.y2 = y2 + viewportY;
    quad.r = quad.g = quad.b = 1.0f;
    quad.useTexture = true;
    quad.textureId = texture;
    quad.layer = 1;
    quad.u1 = 0.0f; quad.v1 = 0.0f;
    quad.u2 = 1.0f; quad.v2 = 1.0f;
    
    overlayQuads.push_back(quad);
}

void SecondRender::DrawGrid(int cellSize, int gridSize) {
    if (!backgroundEnabled || !gridConfig.enabled) return;
    
    gridConfig.cellSize = cellSize;
    gridConfig.gridSize = gridSize;
}

void SecondRender::SetGridConfig(const GridConfig& config) {
    gridConfig = config;
    needBufferUpdate = true;
    needAxesUpdate = true;
}

void SecondRender::SetCamera(const glm::mat4& viewMatrix, const glm::mat4& projMatrix, const glm::vec3& cameraPos) {
    currentViewMat = viewMatrix;
    currentProjMat = projMatrix;
    currentCameraPos = cameraPos;
    cameraMatrixValid = true;
}

std::vector<std::pair<glm::vec3, glm::vec3>> SecondRender::CalculateGridLines() {
    std::vector<std::pair<glm::vec3, glm::vec3>> lines;
    
    if (!gridConfig.infiniteGrid) return lines;
    
    float spacing = GetDynamicSpacing();
    float yOffset = gridConfig.yOffset;
    
    float distance = glm::length(currentCameraPos);
    float worldSize = std::max(200.0f, distance * 1.5f);
    float worldMin = -worldSize;
    float worldMax = worldSize;
    
    float minX = std::max(worldMin, -500.0f);
    float maxX = std::min(worldMax, 500.0f);
    float minZ = std::max(worldMin, -500.0f);
    float maxZ = std::min(worldMax, 500.0f);
    
    minX = floor(minX / spacing) * spacing;
    maxX = ceil(maxX / spacing) * spacing;
    minZ = floor(minZ / spacing) * spacing;
    maxZ = ceil(maxZ / spacing) * spacing;
    
    for (float x = minX; x <= maxX + 0.1f; x += spacing) {
        lines.push_back({glm::vec3(x, yOffset, minZ), glm::vec3(x, yOffset, maxZ)});
    }
    
    for (float z = minZ; z <= maxZ + 0.1f; z += spacing) {
        lines.push_back({glm::vec3(minX, yOffset, z), glm::vec3(maxX, yOffset, z)});
    }
    
    return lines;
}

std::vector<std::pair<glm::vec3, glm::vec3>> SecondRender::CalculateAxesLines() {
    std::vector<std::pair<glm::vec3, glm::vec3>> lines;
    
    float axisLength = gridConfig.fadeDistance;
    float yOffset = gridConfig.yOffset;
    
    // Ось X - бесконечная (красная)
    lines.push_back({glm::vec3(-axisLength, yOffset, 0.0f), glm::vec3(axisLength, yOffset, 0.0f)});
    
    // Ось Z - бесконечная (зеленая)
    lines.push_back({glm::vec3(0.0f, yOffset, -axisLength), glm::vec3(0.0f, yOffset, axisLength)});
    
    return lines;
}

void SecondRender::UpdateGridBuffer() {
    if (!vulkan || !needBufferUpdate) return;
    
    auto lines = CalculateGridLines();
    if (lines.empty()) return;
    
    VkDevice device = vulkan->getDevice();
    
    std::vector<LineVertex> vertices;
    vertices.reserve(lines.size() * 2);
    
    float fadeDist = gridConfig.fadeDistance;
    float centerX = 0.0f;
    float centerZ = 0.0f;
    
    for (const auto& line : lines) {
        float distToCenter;
        if (fabs(line.first.x - line.second.x) < 0.1f) {
            distToCenter = fabs(line.first.x - centerX);
        } else {
            distToCenter = fabs(line.first.z - centerZ);
        }
        
        float alpha = 1.0f - std::min(1.0f, distToCenter / fadeDist);
        if (alpha < 0.05f) continue;
        
        bool isCenter = (fabs(line.first.x) < 1.0f && fabs(line.second.x - line.first.x) > 0.1f) ||
                        (fabs(line.first.z) < 1.0f && fabs(line.second.z - line.first.z) > 0.1f);
        
        float r = isCenter ? gridConfig.centerLineColor[0] : gridConfig.lineColor[0];
        float g = isCenter ? gridConfig.centerLineColor[1] : gridConfig.lineColor[1];
        float b = isCenter ? gridConfig.centerLineColor[2] : gridConfig.lineColor[2];
        
        glm::vec3 color(r * alpha, g * alpha, b * alpha);
        
        vertices.push_back({glm::vec2(line.first.x, line.first.z), color});
        vertices.push_back({glm::vec2(line.second.x, line.second.z), color});
    }
    
    if (vertices.empty()) return;
    
    VkDeviceSize bufferSize = vertices.size() * sizeof(LineVertex);
    
    vkDeviceWaitIdle(device);
    
    if (lineVertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, lineVertexBuffer, nullptr);
        lineVertexBuffer = VK_NULL_HANDLE;
    }
    if (lineVertexBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, lineVertexBufferMemory, nullptr);
        lineVertexBufferMemory = VK_NULL_HANDLE;
    }
    
    VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    
    if (vkCreateBuffer(device, &bufferInfo, nullptr, &lineVertexBuffer) != VK_SUCCESS) {
        std::cerr << "[SecondRender] Failed to create line vertex buffer" << std::endl;
        return;
    }
    
    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(device, lineVertexBuffer, &memReq);
    
    VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = vulkan->findMemoryType(memReq.memoryTypeBits, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    if (vkAllocateMemory(device, &allocInfo, nullptr, &lineVertexBufferMemory) != VK_SUCCESS) {
        std::cerr << "[SecondRender] Failed to allocate line vertex buffer memory" << std::endl;
        vkDestroyBuffer(device, lineVertexBuffer, nullptr);
        lineVertexBuffer = VK_NULL_HANDLE;
        return;
    }
    
    vkBindBufferMemory(device, lineVertexBuffer, lineVertexBufferMemory, 0);
    
    void* data;
    vkMapMemory(device, lineVertexBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), bufferSize);
    vkUnmapMemory(device, lineVertexBufferMemory);
    
    lineVertexCount = (uint32_t)vertices.size();
    needBufferUpdate = false;
    
    std::cout << "[SecondRender] Grid buffer updated: " << lineVertexCount << " vertices" << std::endl;
}

void SecondRender::UpdateAxesBuffer() {
    if (!vulkan || !needAxesUpdate) return;
    
    auto lines = CalculateAxesLines();
    if (lines.empty()) return;
    
    VkDevice device = vulkan->getDevice();
    
    std::vector<LineVertex> vertices;
    vertices.reserve(lines.size() * 2);
    
    float fadeDist = gridConfig.fadeDistance;
    
    for (const auto& line : lines) {
        float distToCenter;
        if (fabs(line.first.x - line.second.x) < 0.1f) {
            distToCenter = fabs(line.first.x);
        } else {
            distToCenter = fabs(line.first.z);
        }
        
        float alpha = 1.0f - std::min(1.0f, distToCenter / fadeDist);
        if (alpha < 0.05f) continue;
        
        glm::vec3 color;
        if (fabs(line.first.z - line.second.z) < 0.1f && line.first.z == 0.0f) {
            color = glm::vec3(gridConfig.axisXColor[0], gridConfig.axisXColor[1], gridConfig.axisXColor[2]);
        } else {
            color = glm::vec3(gridConfig.axisZColor[0], gridConfig.axisZColor[1], gridConfig.axisZColor[2]);
        }
        color *= alpha;
        
        vertices.push_back({glm::vec2(line.first.x, line.first.z), color});
        vertices.push_back({glm::vec2(line.second.x, line.second.z), color});
    }
    
    if (vertices.empty()) return;
    
    VkDeviceSize bufferSize = vertices.size() * sizeof(LineVertex);
    
    vkDeviceWaitIdle(device);
    
    if (axesVertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, axesVertexBuffer, nullptr);
        axesVertexBuffer = VK_NULL_HANDLE;
    }
    if (axesVertexBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, axesVertexBufferMemory, nullptr);
        axesVertexBufferMemory = VK_NULL_HANDLE;
    }
    
    VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    
    if (vkCreateBuffer(device, &bufferInfo, nullptr, &axesVertexBuffer) != VK_SUCCESS) {
        std::cerr << "[SecondRender] Failed to create axes vertex buffer" << std::endl;
        return;
    }
    
    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(device, axesVertexBuffer, &memReq);
    
    VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = vulkan->findMemoryType(memReq.memoryTypeBits, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    if (vkAllocateMemory(device, &allocInfo, nullptr, &axesVertexBufferMemory) != VK_SUCCESS) {
        std::cerr << "[SecondRender] Failed to allocate axes vertex buffer memory" << std::endl;
        vkDestroyBuffer(device, axesVertexBuffer, nullptr);
        axesVertexBuffer = VK_NULL_HANDLE;
        return;
    }
    
    vkBindBufferMemory(device, axesVertexBuffer, axesVertexBufferMemory, 0);
    
    void* data;
    vkMapMemory(device, axesVertexBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), bufferSize);
    vkUnmapMemory(device, axesVertexBufferMemory);
    
    axesVertexCount = (uint32_t)vertices.size();
    needAxesUpdate = false;
}

void SecondRender::RenderInfiniteGrid() {
    if (!gridConfig.enabled || !gridConfig.infiniteGrid || !vulkan) return;
    if (!cameraMatrixValid) return;
    if (linePipeline == VK_NULL_HANDLE) {
        if (!CreateLinePipeline()) return;
    }
    
    if (needBufferUpdate) {
        UpdateGridBuffer();
    }
    
    if (needAxesUpdate && gridConfig.showAxes) {
        UpdateAxesBuffer();
    }
    
    VkCommandBuffer cmdBuffer = vulkan->getCurrentCommandBuffer();
    if (cmdBuffer == VK_NULL_HANDLE) return;
    
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, linePipeline);
    
    VkViewport viewport{};
    viewport.x = (float)viewportX;
    viewport.y = (float)viewportY;
    viewport.width = (float)viewportW;
    viewport.height = (float)viewportH;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
    
    VkRect2D scissor{};
    scissor.offset.x = viewportX;
    scissor.offset.y = viewportY;
    scissor.extent.width = viewportW;
    scissor.extent.height = viewportH;
    vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
    
    vkCmdSetLineWidth(cmdBuffer, gridConfig.lineThickness);
    
    // Используем ТЕ ЖЕ МАТРИЦЫ, что и для модели
    glm::mat4 projView = currentProjMat * currentViewMat;
    vkCmdPushConstants(cmdBuffer, linePipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &projView);
    
    // Рисуем сетку
    if (lineVertexBuffer != VK_NULL_HANDLE && lineVertexCount > 0) {
        VkDeviceSize offsets = 0;
        vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &lineVertexBuffer, &offsets);
        vkCmdDraw(cmdBuffer, lineVertexCount, 1, 0, 0);
    }
    
    // Рисуем оси поверх сетки
    if (gridConfig.showAxes && axesVertexBuffer != VK_NULL_HANDLE && axesVertexCount > 0) {
        VkDeviceSize offsets = 0;
        vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &axesVertexBuffer, &offsets);
        vkCmdDraw(cmdBuffer, axesVertexCount, 1, 0, 0);
    }
}

void SecondRender::DrawTestQuads() {
    testQuadsDirty = true;
}

void SecondRender::RebuildTestQuadsIfNeeded() {
    if (!testQuadsDirty) return;
    
    testBackgroundQuads.clear();
    testOverlayQuads.clear();
    
    Quad2D quad;
    
    quad.useTexture = false;
    quad.textureId = nullptr;
    quad.layer = 0;
    quad.x1 = 200.0f + viewportX;
    quad.y1 = 200.0f + viewportY;
    quad.x2 = 300.0f + viewportX;
    quad.y2 = 300.0f + viewportY;
    quad.r = 0.3f; quad.g = 0.3f; quad.b = 0.4f;
    testBackgroundQuads.push_back(quad);
    
    quad.layer = 1;
    quad.x1 = 50.0f + viewportX;
    quad.y1 = 50.0f + viewportY;
    quad.x2 = 150.0f + viewportX;
    quad.y2 = 150.0f + viewportY;
    quad.r = 1.0f; quad.g = 1.0f; quad.b = 1.0f;
    testOverlayQuads.push_back(quad);
    
    quad.x1 = (viewportW - 150.0f) + viewportX;
    quad.y1 = 50.0f + viewportY;
    quad.x2 = (viewportW - 50.0f) + viewportX;
    quad.y2 = 150.0f + viewportY;
    testOverlayQuads.push_back(quad);
    
    testQuadsDirty = false;
}

void SecondRender::RenderBackground() {
    if (!backgroundEnabled || !vulkan) return;
    
    UpdateViewportRect();
    RebuildTestQuadsIfNeeded();
    
    std::vector<Quad2D> quadsToRender;
    quadsToRender.insert(quadsToRender.end(), testBackgroundQuads.begin(), testBackgroundQuads.end());
    quadsToRender.insert(quadsToRender.end(), backgroundQuads.begin(), backgroundQuads.end());
    
    for (const auto& quad : quadsToRender) {
        if (quad.useTexture && quad.textureId) {
            vulkan->drawImageUV(quad.x1, quad.y1, quad.x2, quad.y2, 
                                (VulkanTexture*)quad.textureId,
                                quad.u1, quad.v1, quad.u2, quad.v2);
        } else {
            vulkan->drawBackground(quad.x1, quad.y1, quad.x2, quad.y2, quad.r, quad.g, quad.b);
        }
    }
    
    backgroundQuads.clear();
}

void SecondRender::RenderOverlay() {
    if (!overlayEnabled || !vulkan) return;
    
    UpdateViewportRect();
    RebuildTestQuadsIfNeeded();
    
    std::vector<Quad2D> quadsToRender;
    quadsToRender.insert(quadsToRender.end(), testOverlayQuads.begin(), testOverlayQuads.end());
    quadsToRender.insert(quadsToRender.end(), overlayQuads.begin(), overlayQuads.end());
    
    for (const auto& quad : quadsToRender) {
        if (quad.useTexture && quad.textureId) {
            vulkan->drawImageUV(quad.x1, quad.y1, quad.x2, quad.y2, 
                                (VulkanTexture*)quad.textureId,
                                quad.u1, quad.v1, quad.u2, quad.v2);
        } else {
            vulkan->drawQuad(quad.x1, quad.y1, quad.x2, quad.y2, quad.r, quad.g, quad.b);
        }
    }
    
    overlayQuads.clear();
}

void SecondRender::ClearBackground() {
    backgroundQuads.clear();
}

void SecondRender::ClearOverlay() {
    overlayQuads.clear();
}

void SecondRender::UpdateScreenSize(int width, int height) {
    screenW = width;
    screenH = height;
    UpdateViewportRect();
    testQuadsDirty = true;
    needBufferUpdate = true;
    needAxesUpdate = true;
}
float SecondRender::GetDynamicSpacing() {
    float baseSpacing = 5.0f;
    float distance = glm::length(currentCameraPos);
    float zoomFactor = std::max(0.5f, std::min(10.0f, distance / 20.0f));
    
    if (zoomFactor < 1.0f) return baseSpacing * 0.5f;
    if (zoomFactor < 2.0f) return baseSpacing;
    if (zoomFactor < 4.0f) return baseSpacing * 2.0f;
    if (zoomFactor < 8.0f) return baseSpacing * 4.0f;
    return baseSpacing * 8.0f;
}

void SecondRender::SetZoomLevel(float zoom) {
    currentZoom = zoom;
    needBufferUpdate = true;
}
void SecondRender::RenderContour(ObjectID objectId, float thickness, float r, float g, float b) {
    if (!vulkan || !gridConfig.enabled) return;
    if (objectId == 0) return;
    
    contourThickness = thickness;
    contourColor[0] = r;
    contourColor[1] = g;
    contourColor[2] = b;
    currentContourObject = objectId;
    
    // Получаем объект из SceneManager
    auto& sm = SceneManager::Instance();
    SceneObject* obj = sm.GetSceneObject(objectId);
    if (!obj || !obj->parser) return;
    
    // Получаем bounding box объекта
    if (!obj->hasBoundingBox) {
        obj->calculateBoundingBox();
    }
    
    glm::vec3 min = obj->boundingBoxMin;
    glm::vec3 max = obj->boundingBoxMax;
    
    // Создаем линии для рамки вокруг объекта (8 угловых точек, 12 линий)
    std::vector<std::pair<glm::vec3, glm::vec3>> contourLines;
    
    // Нижняя грань (Y = min.y)
    contourLines.push_back({glm::vec3(min.x, min.y, min.z), glm::vec3(max.x, min.y, min.z)});
    contourLines.push_back({glm::vec3(max.x, min.y, min.z), glm::vec3(max.x, min.y, max.z)});
    contourLines.push_back({glm::vec3(max.x, min.y, max.z), glm::vec3(min.x, min.y, max.z)});
    contourLines.push_back({glm::vec3(min.x, min.y, max.z), glm::vec3(min.x, min.y, min.z)});
    
    // Верхняя грань (Y = max.y)
    contourLines.push_back({glm::vec3(min.x, max.y, min.z), glm::vec3(max.x, max.y, min.z)});
    contourLines.push_back({glm::vec3(max.x, max.y, min.z), glm::vec3(max.x, max.y, max.z)});
    contourLines.push_back({glm::vec3(max.x, max.y, max.z), glm::vec3(min.x, max.y, max.z)});
    contourLines.push_back({glm::vec3(min.x, max.y, max.z), glm::vec3(min.x, max.y, min.z)});
    
    // Вертикальные линии (соединяем нижнюю и верхнюю грани)
    contourLines.push_back({glm::vec3(min.x, min.y, min.z), glm::vec3(min.x, max.y, min.z)});
    contourLines.push_back({glm::vec3(max.x, min.y, min.z), glm::vec3(max.x, max.y, min.z)});
    contourLines.push_back({glm::vec3(max.x, min.y, max.z), glm::vec3(max.x, max.y, max.z)});
    contourLines.push_back({glm::vec3(min.x, min.y, max.z), glm::vec3(min.x, max.y, max.z)});
    
    // Применяем трансформацию объекта к вершинам рамки
    glm::mat4 worldMat = sm.GetWorldMatrix(objectId);
    
    std::vector<LineVertex> vertices;
    for (const auto& line : contourLines) {
        glm::vec4 p1 = worldMat * glm::vec4(line.first, 1.0f);
        glm::vec4 p2 = worldMat * glm::vec4(line.second, 1.0f);
        
        vertices.push_back({glm::vec2(p1.x, p1.z), glm::vec3(r, g, b)});
        vertices.push_back({glm::vec2(p2.x, p2.z), glm::vec3(r, g, b)});
    }
    
    if (vertices.empty()) return;
    
    // Рисуем контур
    VkCommandBuffer cmdBuffer = vulkan->getCurrentCommandBuffer();
    if (cmdBuffer == VK_NULL_HANDLE) return;
    
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, linePipeline);
    
    VkDeviceSize bufferSize = vertices.size() * sizeof(LineVertex);
    
    // Создаем временный буфер для контура
    VkBuffer tempBuffer;
    VkDeviceMemory tempMemory;
    VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    vkCreateBuffer(vulkan->getDevice(), &bufferInfo, nullptr, &tempBuffer);
    
    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(vulkan->getDevice(), tempBuffer, &memReq);
    VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = vulkan->findMemoryType(memReq.memoryTypeBits, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkAllocateMemory(vulkan->getDevice(), &allocInfo, nullptr, &tempMemory);
    vkBindBufferMemory(vulkan->getDevice(), tempBuffer, tempMemory, 0);
    
    void* data;
    vkMapMemory(vulkan->getDevice(), tempMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), bufferSize);
    vkUnmapMemory(vulkan->getDevice(), tempMemory);
    
    vkCmdSetLineWidth(cmdBuffer, thickness);
    
    VkDeviceSize offsets = 0;
    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &tempBuffer, &offsets);
    vkCmdDraw(cmdBuffer, (uint32_t)vertices.size(), 1, 0, 0);
    
    vkDestroyBuffer(vulkan->getDevice(), tempBuffer, nullptr);
    vkFreeMemory(vulkan->getDevice(), tempMemory, nullptr);
}