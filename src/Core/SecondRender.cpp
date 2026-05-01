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
    , linePipeline(VK_NULL_HANDLE)
    , linePipelineLayout(VK_NULL_HANDLE)
    , lineVertexBuffer(VK_NULL_HANDLE)
    , lineVertexBufferMemory(VK_NULL_HANDLE)
    , lineVertexCount(0)
    , needBufferUpdate(true)
    , lastCamX(0.0f)
    , lastCamZ(0.0f)
{
    gridConfig.cellSize = 50;
    gridConfig.gridSize = 20;
    gridConfig.lineColor[0] = 0.4f;
    gridConfig.lineColor[1] = 0.4f;
    gridConfig.lineColor[2] = 0.45f;
    gridConfig.centerLineColor[0] = 0.8f;
    gridConfig.centerLineColor[1] = 0.8f;
    gridConfig.centerLineColor[2] = 1.0f;
    gridConfig.enabled = true;
    gridConfig.infiniteGrid = true;
    gridConfig.gridSpacing = 20.0f;
    gridConfig.fadeDistance = 200.0f;
    gridConfig.yOffset = 0.0f;
    gridConfig.lineThickness = 1.0f;
    
    std::cout << "[SecondRender] Constructor called" << std::endl;
}

SecondRender::~SecondRender() {
    ClearBackground();
    ClearOverlay();
    DestroyLineResources();
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
    
    overlayQuads.push_back(quad);
}

void SecondRender::SetGridConfig(const GridConfig& config) {
    gridConfig = config;
    needBufferUpdate = true;
}

void SecondRender::SetCamera(const glm::mat4& viewMatrix, const glm::mat4& projMatrix, const glm::vec3& cameraPos) {
    currentViewMat = viewMatrix;
    currentProjMat = projMatrix;
    currentCameraPos = cameraPos;
    cameraMatrixValid = true;
    
    // Проверяем, нужно ли обновлять буфер
    float newCamX = cameraPos.x;
    float newCamZ = cameraPos.z;
    if (fabs(newCamX - lastCamX) > gridConfig.gridSpacing * 0.5f ||
        fabs(newCamZ - lastCamZ) > gridConfig.gridSpacing * 0.5f) {
        needBufferUpdate = true;
        lastCamX = newCamX;
        lastCamZ = newCamZ;
    }
}

std::vector<std::pair<glm::vec3, glm::vec3>> SecondRender::CalculateGridLines() {
    std::vector<std::pair<glm::vec3, glm::vec3>> lines;
    
    if (!gridConfig.infiniteGrid || !cameraMatrixValid) return lines;
    
    float spacing = gridConfig.gridSpacing;
    float yOffset = gridConfig.yOffset;
    
    float camX = currentCameraPos.x;
    float camZ = currentCameraPos.z;
    
    int gridSize = 30;
    int startX = (int)(camX / spacing) - gridSize;
    int endX = (int)(camX / spacing) + gridSize;
    int startZ = (int)(camZ / spacing) - gridSize;
    int endZ = (int)(camZ / spacing) + gridSize;
    
    float range = gridSize * spacing;
    float minX = camX - range;
    float maxX = camX + range;
    float minZ = camZ - range;
    float maxZ = camZ + range;
    
    for (int i = startX; i <= endX; i++) {
        float x = i * spacing;
        
        glm::vec3 start(x, yOffset, minZ);
        glm::vec3 end(x, yOffset, maxZ);
        lines.push_back({start, end});
    }
    
    for (int i = startZ; i <= endZ; i++) {
        float z = i * spacing;
        
        glm::vec3 start(minX, yOffset, z);
        glm::vec3 end(maxX, yOffset, z);
        lines.push_back({start, end});
    }
    
    return lines;
}

void SecondRender::UpdateGridBuffer() {
    if (!vulkan || !cameraMatrixValid || !needBufferUpdate) return;
    
    auto lines = CalculateGridLines();
    if (lines.empty()) return;
    
    VkDevice device = vulkan->getDevice();
    
    std::vector<LineVertex> vertices;
    vertices.reserve(lines.size() * 2);
    
    float camX = currentCameraPos.x;
    float camZ = currentCameraPos.z;
    float fadeDist = gridConfig.fadeDistance;
    
    for (const auto& line : lines) {
        float distToLine;
        if (fabs(line.first.x - line.second.x) < 0.1f) {
            distToLine = fabs(line.first.x - camX);
        } else {
            distToLine = fabs(line.first.z - camZ);
        }
        
        float alpha = 1.0f - std::min(1.0f, distToLine / fadeDist);
        if (alpha < 0.1f) continue;
        
        bool isCenter = (fabs(line.first.x) < 1.0f && fabs(line.second.x - line.first.x) > 0.1f) ||
                        (fabs(line.first.z) < 1.0f && fabs(line.second.z - line.first.z) > 0.1f);
        
        float r = isCenter ? gridConfig.centerLineColor[0] : gridConfig.lineColor[0];
        float g = isCenter ? gridConfig.centerLineColor[1] : gridConfig.lineColor[1];
        float b = isCenter ? gridConfig.centerLineColor[2] : gridConfig.lineColor[2];
        
        glm::vec3 color(r * alpha, g * alpha, b * alpha);
        
        LineVertex v1, v2;
        v1.pos = glm::vec2(line.first.x, line.first.z);
        v1.color = color;
        v2.pos = glm::vec2(line.second.x, line.second.z);
        v2.color = color;
        
        vertices.push_back(v1);
        vertices.push_back(v2);
    }
    
    if (vertices.empty()) return;
    
    VkDeviceSize bufferSize = vertices.size() * sizeof(LineVertex);
    
    // Ждём завершения предыдущих команд перед уничтожением буфера
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
    
    // std::cout << "[SecondRender] Grid buffer updated: " << lineVertexCount << " vertices" << std::endl;
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
    
    if (lineVertexBuffer == VK_NULL_HANDLE || lineVertexCount == 0) return;
    
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
    
    glm::mat4 projView = currentProjMat * currentViewMat;
    vkCmdPushConstants(cmdBuffer, linePipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &projView);
    
    VkDeviceSize offsets = 0;
    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &lineVertexBuffer, &offsets);
    vkCmdDraw(cmdBuffer, lineVertexCount, 1, 0, 0);
}

void SecondRender::RebuildTestQuadsIfNeeded() {
    if (!testQuadsDirty) return;
    testBackgroundQuads.clear();
    testOverlayQuads.clear();
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
        vulkan->drawBackground(quad.x1, quad.y1, quad.x2, quad.y2, quad.r, quad.g, quad.b);
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
        vulkan->drawQuad(quad.x1, quad.y1, quad.x2, quad.y2, quad.r, quad.g, quad.b);
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
}