#include "PostRender.h"
#include "Vulkan.h"
#include <iostream>
#include <cstring>
#include <fstream>

void PostRender::Initialize(Vulkan* vulkan, int screenWidth, int screenHeight) {
    std::cout << "[PostRender] Initialize: " << screenWidth << "x" << screenHeight << std::endl;
    CreateIDBuffer(vulkan, screenWidth, screenHeight);
    CreateIDPipeline(vulkan);
    m_matrixValid = false;
}

void PostRender::Resize(Vulkan* vulkan, int screenWidth, int screenHeight) {
    if (m_idImage != VK_NULL_HANDLE) {
        Destroy(vulkan);
    }
    CreateIDBuffer(vulkan, screenWidth, screenHeight);
    CreateIDPipeline(vulkan);
    m_matrixValid = false;
}

void PostRender::CreateIDBuffer(Vulkan* vulkan, int width, int height) {
    VkDevice device = vulkan->getDevice();
    m_matrixWidth = width;
    m_matrixHeight = height;
    
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = VK_FORMAT_R32_UINT;
    imageInfo.extent.width = (uint32_t)width;
    imageInfo.extent.height = (uint32_t)height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    
    vkCreateImage(device, &imageInfo, nullptr, &m_idImage);
    
    VkMemoryRequirements memReq;
    vkGetImageMemoryRequirements(device, m_idImage, &memReq);
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = vulkan->findMemoryType(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    vkAllocateMemory(device, &allocInfo, nullptr, &m_idImageMemory);
    vkBindImageMemory(device, m_idImage, m_idImageMemory, 0);
    
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_idImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R32_UINT;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;
    
    vkCreateImageView(device, &viewInfo, nullptr, &m_idImageView);
    
    VkAttachmentDescription att{};
    att.format = VK_FORMAT_R32_UINT;
    att.samples = VK_SAMPLE_COUNT_1_BIT;
    att.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    att.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    att.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    att.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    att.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    att.finalLayout = VK_IMAGE_LAYOUT_GENERAL;
    
    VkAttachmentReference colorRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;
    
    VkRenderPassCreateInfo rpInfo{};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpInfo.attachmentCount = 1;
    rpInfo.pAttachments = &att;
    rpInfo.subpassCount = 1;
    rpInfo.pSubpasses = &subpass;
    
    vkCreateRenderPass(device, &rpInfo, nullptr, &m_idRenderPass);
    
    VkFramebufferCreateInfo fbInfo{};
    fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbInfo.renderPass = m_idRenderPass;
    fbInfo.attachmentCount = 1;
    fbInfo.pAttachments = &m_idImageView;
    fbInfo.width = width;
    fbInfo.height = height;
    fbInfo.layers = 1;
    
    vkCreateFramebuffer(device, &fbInfo, nullptr, &m_idFramebuffer);
}

void PostRender::CreateIDPipeline(Vulkan* vulkan) {
    VkDevice device = vulkan->getDevice();
    
    const char* idVertCode = R"(#version 450
layout(location = 0) in vec3 inPosition;
layout(push_constant) uniform PushConstants {
    mat4 mvp;
    uint objectID;
} pc;
layout(location = 0) out uint outObjectID;
void main() {
    gl_Position = pc.mvp * vec4(inPosition, 1.0);
    outObjectID = pc.objectID;
})";
    
    const char* idFragCode = R"(#version 450
layout(location = 0) in flat uint inObjectID;
layout(location = 0) out uvec4 outColor;
void main() {
    outColor = uvec4(inObjectID, 0, 0, 1);
})";
    
    system("mkdir autoshadertest 2>nul");
    
    std::ofstream vertFile("autoshadertest/id_vert.vert");
    vertFile << idVertCode;
    vertFile.close();
    
    std::ofstream fragFile("autoshadertest/id_frag.frag");
    fragFile << idFragCode;
    fragFile.close();
    
    system("glslc autoshadertest/id_vert.vert -o autoshadertest/id_vert.spv");
    system("glslc autoshadertest/id_frag.frag -o autoshadertest/id_frag.spv");
    
    VkShaderModule vertModule = vulkan->createShaderModule("autoshadertest/id_vert.spv");
    VkShaderModule fragModule = vulkan->createShaderModule("autoshadertest/id_frag.spv");
    
    auto bindingDesc = VkVertexInputBindingDescription{0, 32, VK_VERTEX_INPUT_RATE_VERTEX};
    auto attrDesc = std::array<VkVertexInputAttributeDescription, 1>{
        VkVertexInputAttributeDescription{0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0}
    };
    
    VkPipelineVertexInputStateCreateInfo vi{};
    vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vi.vertexBindingDescriptionCount = 1;
    vi.pVertexBindingDescriptions = &bindingDesc;
    vi.vertexAttributeDescriptionCount = (uint32_t)attrDesc.size();
    vi.pVertexAttributeDescriptions = attrDesc.data();
    
    VkPipelineInputAssemblyStateCreateInfo ia{};
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    
    VkPipelineRasterizationStateCreateInfo raster{};
    raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.cullMode = VK_CULL_MODE_BACK_BIT;
    raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster.lineWidth = 1.0f;
    
    VkPipelineMultisampleStateCreateInfo ms{};
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    
    VkPipelineColorBlendAttachmentState blendAtt{};
    blendAtt.colorWriteMask = 0xF;
    blendAtt.blendEnable = VK_FALSE;
    
    VkPipelineColorBlendStateCreateInfo cb{};
    cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cb.attachmentCount = 1;
    cb.pAttachments = &blendAtt;
    
    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic{};
    dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic.dynamicStateCount = 2;
    dynamic.pDynamicStates = dynamicStates;
    
    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushRange.offset = 0;
    pushRange.size = sizeof(glm::mat4) + sizeof(uint32_t);
    
    VkPipelineLayoutCreateInfo plInfo{};
    plInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plInfo.setLayoutCount = 0;
    plInfo.pushConstantRangeCount = 1;
    plInfo.pPushConstantRanges = &pushRange;
    
    vkCreatePipelineLayout(device, &plInfo, nullptr, &m_idPipelineLayout);
    
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
    
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = stages;
    pipelineInfo.pVertexInputState = &vi;
    pipelineInfo.pInputAssemblyState = &ia;
    pipelineInfo.pViewportState = nullptr;
    pipelineInfo.pRasterizationState = &raster;
    pipelineInfo.pMultisampleState = &ms;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &cb;
    pipelineInfo.pDynamicState = &dynamic;
    pipelineInfo.layout = m_idPipelineLayout;
    pipelineInfo.renderPass = m_idRenderPass;
    pipelineInfo.subpass = 0;
    
    vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_idPipeline);
    
    vkDestroyShaderModule(device, vertModule, nullptr);
    vkDestroyShaderModule(device, fragModule, nullptr);
}

void PostRender::BeginIDPass(VkCommandBuffer cmdBuffer, VkRenderPass renderPass, VkFramebuffer framebuffer, int width, int height) {
    VkClearValue clearValue;
    clearValue.color.uint32[0] = 0;
    clearValue.color.uint32[1] = 0;
    clearValue.color.uint32[2] = 0;
    clearValue.color.uint32[3] = 0;
    
    VkRenderPassBeginInfo rpInfo{};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpInfo.renderPass = m_idRenderPass;
    rpInfo.framebuffer = m_idFramebuffer;
    rpInfo.renderArea.offset = {0, 0};
    rpInfo.renderArea.extent = { (uint32_t)width, (uint32_t)height };
    rpInfo.clearValueCount = 1;
    rpInfo.pClearValues = &clearValue;
    
    vkCmdBeginRenderPass(cmdBuffer, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_idPipeline);
    
    VkViewport viewport{0, 0, (float)width, (float)height, 0, 1};
    vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
    
    VkRect2D scissor{{0, 0}, { (uint32_t)width, (uint32_t)height }};
    vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
}

void PostRender::EndIDPass(VkCommandBuffer cmdBuffer) {
    vkCmdEndRenderPass(cmdBuffer);
}

void PostRender::ReadIDBuffer(Vulkan* vulkan) {
    VkDevice device = vulkan->getDevice();
    
    if (m_idImage == VK_NULL_HANDLE || m_matrixWidth <= 0 || m_matrixHeight <= 0) return;
    
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;
    VkDeviceSize bufferSize = m_matrixWidth * m_matrixHeight * sizeof(uint32_t);
    
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    
    if (vkCreateBuffer(device, &bufferInfo, nullptr, &stagingBuffer) != VK_SUCCESS) return;
    
    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(device, stagingBuffer, &memReq);
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = vulkan->findMemoryType(memReq.memoryTypeBits, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    if (vkAllocateMemory(device, &allocInfo, nullptr, &stagingMemory) != VK_SUCCESS) {
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        return;
    }
    
    vkBindBufferMemory(device, stagingBuffer, stagingMemory, 0);
    
    VkCommandBuffer cmdBuffer = vulkan->beginSingleTimeCommands();
    
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = m_idImage;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    
    vkCmdPipelineBarrier(cmdBuffer,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);
    
    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent.width = (uint32_t)m_matrixWidth;
    region.imageExtent.height = (uint32_t)m_matrixHeight;
    region.imageExtent.depth = 1;
    
    vkCmdCopyImageToBuffer(cmdBuffer, m_idImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           stagingBuffer, 1, &region);
    
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    
    vkCmdPipelineBarrier(cmdBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);
    
    vulkan->endSingleTimeCommands(cmdBuffer);
    
    uint32_t* pixelData = new uint32_t[m_matrixWidth * m_matrixHeight];
    void* mappedData;
    vkMapMemory(device, stagingMemory, 0, bufferSize, 0, &mappedData);
    memcpy(pixelData, mappedData, bufferSize);
    vkUnmapMemory(device, stagingMemory);
    
    m_idMatrix.clear();
    m_idMatrix.resize(m_matrixHeight);
    for (int y = 0; y < m_matrixHeight; y++) {
        m_idMatrix[y].resize(m_matrixWidth);
        for (int x = 0; x < m_matrixWidth; x++) {
            m_idMatrix[y][x] = pixelData[y * m_matrixWidth + x];
        }
    }
    
    m_matrixValid = true;
    
    delete[] pixelData;
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingMemory, nullptr);
}

void PostRender::UpdateAndMaybeHighlight(Vulkan* vulkan, int screenWidth, int screenHeight) {
    m_frameCounter++;
    
    if (m_frameCounter % 60 == 0) {
        ReadIDBuffer(vulkan);
    }
    
    if (m_matrixValid && !m_idMatrix.empty()) {
        int height = (int)m_idMatrix.size();
        int width = (int)m_idMatrix[0].size();
        
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                if (m_idMatrix[y][x] == m_lastHighlightedID) {
                    float screenX = (float)x / width * screenWidth;
                    float screenY = (float)y / height * screenHeight;
                    float pixelW = (float)screenWidth / width;
                    float pixelH = (float)screenHeight / height;
                    
                    vulkan->drawQuad(screenX, screenY, screenX + pixelW, screenY + pixelH, 1.0f, 0.0f, 0.0f);
                }
            }
        }
    }
}

void PostRender::DrawTestSquare(Vulkan* vulkan, int screenWidth, int screenHeight) {
    int centerX = screenWidth / 2;
    int centerY = screenHeight / 2;
    int size = 100;
    
    vulkan->drawQuad(centerX - size/2, centerY - size/2, 
                     centerX + size/2, centerY + size/2, 
                     1.0f, 0.0f, 0.0f);
}

void PostRender::Destroy(Vulkan* vulkan) {
    VkDevice device = vulkan->getDevice();
    
    if (m_idImageView) vkDestroyImageView(device, m_idImageView, nullptr);
    if (m_idImage) vkDestroyImage(device, m_idImage, nullptr);
    if (m_idImageMemory) vkFreeMemory(device, m_idImageMemory, nullptr);
    if (m_idPipeline) vkDestroyPipeline(device, m_idPipeline, nullptr);
    if (m_idPipelineLayout) vkDestroyPipelineLayout(device, m_idPipelineLayout, nullptr);
    if (m_idRenderPass) vkDestroyRenderPass(device, m_idRenderPass, nullptr);
    if (m_idFramebuffer) vkDestroyFramebuffer(device, m_idFramebuffer, nullptr);
    
    m_idImage = VK_NULL_HANDLE;
    m_idImageView = VK_NULL_HANDLE;
    m_idImageMemory = VK_NULL_HANDLE;
    m_idPipeline = VK_NULL_HANDLE;
    m_idPipelineLayout = VK_NULL_HANDLE;
    m_idRenderPass = VK_NULL_HANDLE;
    m_idFramebuffer = VK_NULL_HANDLE;
}