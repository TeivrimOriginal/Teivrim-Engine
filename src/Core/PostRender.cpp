// PostRender.cpp
#include "PostRender.h"
#include "Vulkan.h"
#include <iostream>
#include <fstream>

static std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) return {};
    size_t fileSize = file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    return buffer;
}

PostRender::PostRender() {
    std::cout << "[PostRender] Constructor called" << std::endl;
}

PostRender::~PostRender() {
    Shutdown();
}

void PostRender::Initialize(Vulkan* vulkan, int width, int height) {
    if (!vulkan) return;
    
    m_width = width;
    m_height = height;
    
    CreateShaders(vulkan);
    CreatePipeline(vulkan);
    CreateVertexBuffer(vulkan);
    
    m_initialized = true;
    std::cout << "[PostRender] Initialized (" << width << "x" << height << ")" << std::endl;
}

void PostRender::Shutdown() {
    m_initialized = false;
}

void PostRender::Resize(int width, int height) {
    m_width = width;
    m_height = height;
}

void PostRender::SetOutlineColor(float r, float g, float b) {
    m_outlineColor[0] = r;
    m_outlineColor[1] = g;
    m_outlineColor[2] = b;
}

void PostRender::SetOutlineThickness(int thickness) {
    m_outlineThickness = thickness;
    if (m_outlineThickness < 1) m_outlineThickness = 1;
    if (m_outlineThickness > 20) m_outlineThickness = 20;
    std::cout << "[PostRender] Outline thickness set to: " << m_outlineThickness << " pixels" << std::endl;
}

void PostRender::SetEnabled(bool enabled) {
    m_enabled = enabled;
}

void PostRender::Process(Vulkan* vulkan, ObjectID selectedObjectId) {
    if (!m_initialized || !m_enabled) return;
    if (!vulkan || !vulkan->isInitialized()) return;
    
    m_selectedID = selectedObjectId;
}
bool PostRender::CreateShaders(Vulkan* vulkan) {
    const char* vertCode = R"(#version 450
layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inTexCoord;
layout(location = 0) out vec2 fragTexCoord;
void main() {
    gl_Position = vec4(inPos, 0.0, 1.0);
    fragTexCoord = inTexCoord;
}
)";
    
const char* fragCode = R"(#version 450
layout(binding = 0) uniform sampler2D idTexture;  // sampler2D вместо usampler2D
layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushConstants {
    uint selectedID;
    vec3 outlineColor;
    vec2 texelSize;
    int thickness;
} pc;

void main() {
    float centerVal = texture(idTexture, fragTexCoord).r;
    uint centerID = uint(centerVal * 255.0);
    
    if (centerID == pc.selectedID) {
        outColor = vec4(pc.outlineColor, 1.0);
    } else {
        outColor = vec4(0.0, 0.0, 0.0, 0.0);
    }
}
)";
    std::ofstream vertFile("autoshadertest/post_vert.vert");
    vertFile << vertCode;
    vertFile.close();
    
    std::ofstream fragFile("autoshadertest/post_frag.frag");
    fragFile << fragCode;
    fragFile.close();
    
    system("glslc autoshadertest/post_vert.vert -o autoshadertest/post_vert.spv");
    system("glslc autoshadertest/post_frag.frag -o autoshadertest/post_frag.spv");
    
    return true;
}

void PostRender::CreatePipeline(Vulkan* vulkan) {
    VkDevice device = vulkan->getDevice();
    
    auto vertCode = readFile("autoshadertest/post_vert.spv");
    auto fragCode = readFile("autoshadertest/post_frag.spv");
    
    if (vertCode.empty() || fragCode.empty()) {
        std::cerr << "[PostRender] Failed to load shaders" << std::endl;
        return;
    }
    
    VkShaderModuleCreateInfo moduleInfo{};
    moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    
    VkShaderModule vertModule, fragModule;
    moduleInfo.codeSize = vertCode.size();
    moduleInfo.pCode = reinterpret_cast<const uint32_t*>(vertCode.data());
    vkCreateShaderModule(device, &moduleInfo, nullptr, &vertModule);
    
    moduleInfo.codeSize = fragCode.size();
    moduleInfo.pCode = reinterpret_cast<const uint32_t*>(fragCode.data());
    vkCreateShaderModule(device, &moduleInfo, nullptr, &fragModule);
    
    auto bindingDesc = VkVertexInputBindingDescription{0, sizeof(ScreenQuadVertex), VK_VERTEX_INPUT_RATE_VERTEX};
    auto attrDesc = std::array<VkVertexInputAttributeDescription, 2>{
        VkVertexInputAttributeDescription{0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(ScreenQuadVertex, pos)},
        VkVertexInputAttributeDescription{1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(ScreenQuadVertex, uv)}
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
    
    VkViewport viewport{0, 0, (float)m_width, (float)m_height, 0, 1};
    VkRect2D scissor{{0, 0}, {(uint32_t)m_width, (uint32_t)m_height}};
    
    VkPipelineViewportStateCreateInfo vpState{};
    vpState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vpState.viewportCount = 1;
    vpState.pViewports = &viewport;
    vpState.scissorCount = 1;
    vpState.pScissors = &scissor;
    
    VkPipelineRasterizationStateCreateInfo raster{};
    raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.cullMode = VK_CULL_MODE_NONE;
    raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster.lineWidth = 1.0f;
    
    VkPipelineMultisampleStateCreateInfo ms{};
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_FALSE;
    depthStencil.depthWriteEnable = VK_FALSE;
    
    VkPipelineColorBlendAttachmentState blendAtt{};
    blendAtt.blendEnable = VK_TRUE;
    blendAtt.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendAtt.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAtt.colorBlendOp = VK_BLEND_OP_ADD;
    blendAtt.colorWriteMask = 0xF;
    
    VkPipelineColorBlendStateCreateInfo cb{};
    cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cb.attachmentCount = 1;
    cb.pAttachments = &blendAtt;
    
    // Push constant - увеличиваем размер до 64 (больше чем нужно)
    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    pushRange.offset = 0;
    pushRange.size = 64;  // Достаточно большой размер
    
    // Дескриптор для ID текстуры
    VkDescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &binding;
    
    vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &m_descLayout);
    
    VkPipelineLayoutCreateInfo plInfo{};
    plInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plInfo.setLayoutCount = 1;
    plInfo.pSetLayouts = &m_descLayout;
    plInfo.pushConstantRangeCount = 1;
    plInfo.pPushConstantRanges = &pushRange;
    
    vkCreatePipelineLayout(device, &plInfo, nullptr, &m_pipelineLayout);
    
    // Дескриптор сет
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 1;
    
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 1;
    
    VkDescriptorPool descPool;
    vkCreateDescriptorPool(device, &poolInfo, nullptr, &descPool);
    
    VkDescriptorSetAllocateInfo descAlloc{};
    descAlloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descAlloc.descriptorPool = descPool;
    descAlloc.descriptorSetCount = 1;
    descAlloc.pSetLayouts = &m_descLayout;
    
    vkAllocateDescriptorSets(device, &descAlloc, &m_descSet);
    
    // Связываем с ID текстурой
    VkDescriptorImageInfo imageInfo{};
    imageInfo.sampler = vulkan->GetIDBufferSampler();
    imageInfo.imageView = vulkan->GetIDImageView();
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    
    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = m_descSet;
    write.dstBinding = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &imageInfo;
    
    vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    
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
    pipelineInfo.pViewportState = &vpState;
    pipelineInfo.pRasterizationState = &raster;
    pipelineInfo.pMultisampleState = &ms;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &cb;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = vulkan->getRenderPass();
    pipelineInfo.subpass = 0;
    
    vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline);
    
    vkDestroyShaderModule(device, vertModule, nullptr);
    vkDestroyShaderModule(device, fragModule, nullptr);
    
    std::cout << "[PostRender] Pipeline created" << std::endl;
}

void PostRender::CreateVertexBuffer(Vulkan* vulkan) {
    VkDevice device = vulkan->getDevice();
    
    std::vector<ScreenQuadVertex> vertices = {
        {{-1.0f, -1.0f}, {0.0f, 1.0f}},
        {{ 1.0f, -1.0f}, {1.0f, 1.0f}},
        {{-1.0f,  1.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f}, {1.0f, 1.0f}},
        {{ 1.0f,  1.0f}, {1.0f, 0.0f}},
        {{-1.0f,  1.0f}, {0.0f, 0.0f}}
    };
    
    VkDeviceSize bufferSize = vertices.size() * sizeof(ScreenQuadVertex);
    
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    
    vkCreateBuffer(device, &bufferInfo, nullptr, &m_vertexBuffer);
    
    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(device, m_vertexBuffer, &memReq);
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = vulkan->findMemoryType(memReq.memoryTypeBits, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    vkAllocateMemory(device, &allocInfo, nullptr, &m_vertexBufferMemory);
    vkBindBufferMemory(device, m_vertexBuffer, m_vertexBufferMemory, 0);
    
    void* data;
    vkMapMemory(device, m_vertexBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), bufferSize);
    vkUnmapMemory(device, m_vertexBufferMemory);
}

void PostRender::Render(Vulkan* vulkan, VkCommandBuffer cmdBuffer, uint32_t screenWidth, uint32_t screenHeight) {
    if (!m_initialized || !m_enabled || m_selectedID == 0) return;
    if (m_pipeline == VK_NULL_HANDLE) return;
    
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                           m_pipelineLayout, 0, 1, &m_descSet, 0, nullptr);
    
    struct PushConstants {
        uint32_t selectedID;
        float outlineColor[3];
        float texelSize[2];
        int thickness;
    } pc;
    
    pc.selectedID = m_selectedID;
    pc.outlineColor[0] = 1.0f;
    pc.outlineColor[1] = 0.0f;
    pc.outlineColor[2] = 0.0f;
    pc.texelSize[0] = 1.0f / screenWidth;
    pc.texelSize[1] = 1.0f / screenHeight;
    pc.thickness = 3;
    
    vkCmdPushConstants(cmdBuffer, m_pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pc), &pc);
    
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &m_vertexBuffer, &offset);
    vkCmdDraw(cmdBuffer, 6, 1, 0, 0);
}