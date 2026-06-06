#include "AbstractRender.h"
#include "Vulkan.h"
#include "Win32/rendererw.h"
#include "PostRender.h"
#include "../SecondComplexity/Scene/SceneManager.h"
#include <iostream>

AbstractRender::AbstractRender()
    : m_currentAPI(RenderAPIType::OPENGL)
    , m_initialized(false)
    , m_vulkan(nullptr)
    , m_opengl(nullptr)
    , m_width(0)
    , m_height(0)
    , m_hwnd(nullptr)
{
}

AbstractRender::~AbstractRender()
{
    Shutdown();
}

AbstractRender& AbstractRender::Instance()
{
    static AbstractRender instance;
    return instance;
}

bool AbstractRender::Initialize(RenderAPIType api, InitialWin32* window, int width, int height)
{
    Shutdown();
    
    m_currentAPI = api;
    m_width = width;
    m_height = height;
    m_hwnd = window->getHWND();
    
    if (api == RenderAPIType::VULKAN)
    {
        m_vulkan = new Vulkan(m_hwnd, width, height);
        if (!m_vulkan || !m_vulkan->isInitialized())
        {
            delete m_vulkan;
            m_vulkan = nullptr;
            std::cerr << "[AbstractRender] Failed to initialize Vulkan" << std::endl;
            return false;
        }
        m_vulkan->setup2D(width, height);
        std::cout << "[AbstractRender] Vulkan initialized" << std::endl;
    }
    else
    {
        m_opengl = new RendererW();
        if (!m_opengl->initialize(window))
        {
            delete m_opengl;
            m_opengl = nullptr;
            std::cerr << "[AbstractRender] Failed to initialize OpenGL" << std::endl;
            return false;
        }
        m_opengl->initShaders();
        std::cout << "[AbstractRender] OpenGL initialized" << std::endl;
    }
    
    m_initialized = true;
    return true;
}

void AbstractRender::Shutdown()
{
    if (m_vulkan)
    {
        delete m_vulkan;
        m_vulkan = nullptr;
    }
    if (m_opengl)
    {
        m_opengl->cleanup();
        delete m_opengl;
        m_opengl = nullptr;
    }
    m_initialized = false;
}

void AbstractRender::BeginFrame()
{
    if (!m_initialized) return;
    
    if (m_currentAPI == RenderAPIType::VULKAN && m_vulkan)
    {
        m_vulkan->beginFrame();
    }
    else if (m_currentAPI == RenderAPIType::OPENGL && m_opengl)
    {
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
}

void AbstractRender::EndFrame()
{
    if (!m_initialized) return;
    
    if (m_currentAPI == RenderAPIType::VULKAN && m_vulkan)
    {
        m_vulkan->endFrame();
    }
}

void AbstractRender::Present()
{
    if (!m_initialized) return;
    
    if (m_currentAPI == RenderAPIType::VULKAN && m_vulkan)
    {
        m_vulkan->present();
    }
    else if (m_currentAPI == RenderAPIType::OPENGL && m_opengl && m_hwnd)
    {
        SwapBuffers(GetDC(m_hwnd));
    }
}

void AbstractRender::Setup2D(int width, int height)
{
    m_width = width;
    m_height = height;
    
    if (m_currentAPI == RenderAPIType::VULKAN && m_vulkan)
    {
        m_vulkan->setup2D(width, height);
    }
}

void AbstractRender::DrawBackground(float x1, float y1, float x2, float y2, float r, float g, float b)
{
    if (!m_initialized) return;
    
    if (m_currentAPI == RenderAPIType::VULKAN && m_vulkan)
    {
        m_vulkan->drawBackground(x1, y1, x2, y2, r, g, b);
    }
    else if (m_currentAPI == RenderAPIType::OPENGL && m_opengl)
    {
        m_opengl->drawQuad(x1, y1, x2, y2, r, g, b);
    }
}

void AbstractRender::DrawQuad(float x1, float y1, float x2, float y2, float r, float g, float b)
{
    if (!m_initialized) return;
    
    if (m_currentAPI == RenderAPIType::VULKAN && m_vulkan)
    {
        m_vulkan->drawQuad(x1, y1, x2, y2, r, g, b);
    }
    else if (m_currentAPI == RenderAPIType::OPENGL && m_opengl)
    {
        m_opengl->drawQuad(x1, y1, x2, y2, r, g, b);
    }
}

void AbstractRender::DrawText(int x, int y, const std::string& text, float r, float g, float b)
{
    if (!m_initialized) return;
    
    if (m_currentAPI == RenderAPIType::VULKAN && m_vulkan)
    {
        m_vulkan->drawText(x, y, text, r, g, b);
    }
    else if (m_currentAPI == RenderAPIType::OPENGL && m_opengl)
    {
        m_opengl->drawText(x, y, text, r, g, b);
    }
}

void AbstractRender::DrawTextCentered(int x, int y, int w, int h, const std::string& text, float r, float g, float b)
{
    if (!m_initialized) return;
    
    if (m_currentAPI == RenderAPIType::VULKAN && m_vulkan)
    {
        m_vulkan->drawTextCentered(x, y, w, h, text, r, g, b);
    }
    else if (m_currentAPI == RenderAPIType::OPENGL && m_opengl)
    {
        float textWidth = text.length() * 8;
        float textHeight = 16;
        int centerX = x + (int)((w - textWidth) / 2);
        int centerY = y + (int)((h - textHeight) / 2);
        m_opengl->drawText(centerX, centerY, text, r, g, b);
    }
}

void AbstractRender::DrawImage(float x1, float y1, float x2, float y2, VulkanTexture* texture)
{
    if (!m_initialized) return;
    
    if (m_currentAPI == RenderAPIType::VULKAN && m_vulkan)
    {
        m_vulkan->drawImage(x1, y1, x2, y2, texture);
    }
}

void AbstractRender::DrawImageUV(float x1, float y1, float x2, float y2, VulkanTexture* texture, float u1, float v1, float u2, float v2)
{
    if (!m_initialized) return;
    
    if (m_currentAPI == RenderAPIType::VULKAN && m_vulkan)
    {
        m_vulkan->drawImageUV(x1, y1, x2, y2, texture, u1, v1, u2, v2);
    }
}

VulkanTexture* AbstractRender::LoadUIImage(const std::string& filepath)
{
    if (!m_initialized || m_currentAPI != RenderAPIType::VULKAN || !m_vulkan) return nullptr;
    return m_vulkan->loadUIImage(filepath);
}

VulkanTexture* AbstractRender::LoadUIImageFromData(unsigned char* data, int width, int height, int channels)
{
    if (!m_initialized || m_currentAPI != RenderAPIType::VULKAN || !m_vulkan) return nullptr;
    return m_vulkan->loadUIImageFromData(data, width, height, channels);
}

void AbstractRender::FreeUIImage(VulkanTexture* texture)
{
    if (!m_initialized || m_currentAPI != RenderAPIType::VULKAN || !m_vulkan) return;
    m_vulkan->freeUIImage(texture);
}

int AbstractRender::AddModel(const std::string& name, const std::vector<class StandardMesh>& meshes)
{
    if (!m_initialized) return -1;
    
    if (m_currentAPI == RenderAPIType::VULKAN && m_vulkan)
    {
        return m_vulkan->addModel(name, meshes);
    }
    return -1;
}

void AbstractRender::RemoveModel(const std::string& name)
{
    if (!m_initialized) return;
    
    if (m_currentAPI == RenderAPIType::VULKAN && m_vulkan)
    {
        m_vulkan->removeModel(name);
    }
}

void AbstractRender::ClearModels()
{
    if (!m_initialized) return;
    
    if (m_currentAPI == RenderAPIType::VULKAN && m_vulkan)
    {
        m_vulkan->clearModels();
    }
}

void AbstractRender::RenderAllModels()
{
    if (!m_initialized) return;
    
    if (m_currentAPI == RenderAPIType::VULKAN && m_vulkan)
    {
        m_vulkan->renderAllModels();
    }
}

void AbstractRender::RenderModel(const std::string& name)
{
    if (!m_initialized) return;
    
    if (m_currentAPI == RenderAPIType::VULKAN && m_vulkan)
    {
        m_vulkan->renderModel(name);
    }
}

void AbstractRender::SetModelTransform(const std::string& name, const glm::mat4& transform)
{
    if (!m_initialized) return;
    
    if (m_currentAPI == RenderAPIType::VULKAN && m_vulkan)
    {
        m_vulkan->setModelTransform(name, transform);
    }
}

void AbstractRender::SetViewMatrix(const glm::mat4& view)
{
    if (!m_initialized) return;
    
    if (m_currentAPI == RenderAPIType::VULKAN && m_vulkan)
    {
        m_vulkan->setViewMatrix(view);
    }
}

void AbstractRender::SetProjectionMatrix(const glm::mat4& proj)
{
    if (!m_initialized) return;
    
    if (m_currentAPI == RenderAPIType::VULKAN && m_vulkan)
    {
        m_vulkan->setProjectionMatrix(proj);
    }
}

void AbstractRender::RecreateSwapchain()
{
    if (!m_initialized) return;
    
    if (m_currentAPI == RenderAPIType::VULKAN && m_vulkan)
    {
        m_vulkan->recreateSwapchain();
    }
}

void* AbstractRender::GetDevice() const
{
    if (!m_initialized || m_currentAPI != RenderAPIType::VULKAN || !m_vulkan) return nullptr;
    return m_vulkan->getDevice();
}

void* AbstractRender::GetRenderPass() const
{
    if (!m_initialized || m_currentAPI != RenderAPIType::VULKAN || !m_vulkan) return nullptr;
    return m_vulkan->getRenderPass();
}

void* AbstractRender::GetCurrentCommandBuffer() const
{
    if (!m_initialized || m_currentAPI != RenderAPIType::VULKAN || !m_vulkan) return nullptr;
    return m_vulkan->getCurrentCommandBuffer();
}

void* AbstractRender::BeginSingleTimeCommands()
{
    if (!m_initialized || m_currentAPI != RenderAPIType::VULKAN || !m_vulkan) return nullptr;
    return m_vulkan->beginSingleTimeCommands();
}

void AbstractRender::EndSingleTimeCommands(void* cmdBuffer)
{
    if (!m_initialized || m_currentAPI != RenderAPIType::VULKAN || !m_vulkan) return;
    m_vulkan->endSingleTimeCommands((VkCommandBuffer)cmdBuffer);
}

uint32_t AbstractRender::FindMemoryType(uint32_t typeFilter, uint32_t properties)
{
    if (!m_initialized || m_currentAPI != RenderAPIType::VULKAN || !m_vulkan) return 0;
    return m_vulkan->findMemoryType(typeFilter, properties);
}

void AbstractRender::SetViewportClip(int x, int y, int w, int h)
{
    if (!m_initialized) return;
    
    if (m_currentAPI == RenderAPIType::VULKAN && m_vulkan)
    {
        m_vulkan->SetViewportClip(x, y, w, h);
    }
    else if (m_currentAPI == RenderAPIType::OPENGL && m_opengl)
    {
        glEnable(GL_SCISSOR_TEST);
        glScissor(x, m_height - (y + h), w, h);
    }
}

void AbstractRender::DisableViewportClip()
{
    if (!m_initialized) return;
    
    if (m_currentAPI == RenderAPIType::VULKAN && m_vulkan)
    {
        m_vulkan->DisableViewportClip();
    }
    else if (m_currentAPI == RenderAPIType::OPENGL && m_opengl)
    {
        glDisable(GL_SCISSOR_TEST);
    }
}

bool AbstractRender::IsViewportClippingEnabled() const
{
    if (!m_initialized) return false;
    
    if (m_currentAPI == RenderAPIType::VULKAN && m_vulkan)
    {
        return m_vulkan->IsViewportClippingEnabled();
    }
    return false;
}

void AbstractRender::FlushBackground()
{
    if (!m_initialized) return;
    
    if (m_currentAPI == RenderAPIType::VULKAN && m_vulkan)
    {
        m_vulkan->FlushBackground();
    }
}

void AbstractRender::RenderBackground()
{
    if (!m_initialized) return;
    
    if (m_currentAPI == RenderAPIType::VULKAN && m_vulkan)
    {
        m_vulkan->renderBackground();
    }
}

void AbstractRender::RenderScene()
{
    if (!m_initialized) return;
    
    if (m_currentAPI == RenderAPIType::VULKAN && m_vulkan)
    {
        m_vulkan->renderScene();
    }
}

void AbstractRender::RenderOverlay()
{
    if (!m_initialized) return;
    
    if (m_currentAPI == RenderAPIType::VULKAN && m_vulkan)
    {
        m_vulkan->renderOverlay();
    }
}

glm::mat4 AbstractRender::GetViewMatrix() const
{
    if (!m_initialized || m_currentAPI != RenderAPIType::VULKAN || !m_vulkan) return glm::mat4(1.0f);
    return m_vulkan->getViewMatrix();
}

glm::mat4 AbstractRender::GetProjectionMatrix() const
{
    if (!m_initialized || m_currentAPI != RenderAPIType::VULKAN || !m_vulkan) return glm::mat4(1.0f);
    return m_vulkan->getProjectionMatrix();
}

void AbstractRender::RenderGrid(const glm::mat4& viewMatrix, const glm::mat4& projMatrix)
{
    if (!m_initialized) return;
    
    if (m_currentAPI == RenderAPIType::VULKAN && m_vulkan)
    {
        m_vulkan->renderGrid(viewMatrix, projMatrix);
    }
}

void AbstractRender::SetGridEnabled(bool enabled)
{
    if (!m_initialized) return;
    
    if (m_currentAPI == RenderAPIType::VULKAN && m_vulkan)
    {
        m_vulkan->setGridEnabled(enabled);
    }
}

void AbstractRender::SetGridSpacing(float spacing)
{
    if (!m_initialized) return;
    
    if (m_currentAPI == RenderAPIType::VULKAN && m_vulkan)
    {
        m_vulkan->setGridSpacing(spacing);
    }
}

void AbstractRender::SetGridFadeDistance(float distance)
{
    if (!m_initialized) return;
    
    if (m_currentAPI == RenderAPIType::VULKAN && m_vulkan)
    {
        m_vulkan->setGridFadeDistance(distance);
    }
}

void AbstractRender::SetGridLineColor(float r, float g, float b)
{
    if (!m_initialized) return;
    
    if (m_currentAPI == RenderAPIType::VULKAN && m_vulkan)
    {
        m_vulkan->setGridLineColor(r, g, b);
    }
}

void AbstractRender::SetGridCenterLineColor(float r, float g, float b)
{
    if (!m_initialized) return;
    
    if (m_currentAPI == RenderAPIType::VULKAN && m_vulkan)
    {
        m_vulkan->setGridCenterLineColor(r, g, b);
    }
}

bool AbstractRender::IsGridEnabled() const
{
    if (!m_initialized || m_currentAPI != RenderAPIType::VULKAN || !m_vulkan) return false;
    return m_vulkan->isGridEnabled();
}

float AbstractRender::GetGridSpacing() const
{
    if (!m_initialized || m_currentAPI != RenderAPIType::VULKAN || !m_vulkan) return 20.0f;
    return m_vulkan->getGridSpacing();
}

void* AbstractRender::GetIDImage() const
{
    if (!m_initialized || m_currentAPI != RenderAPIType::VULKAN || !m_vulkan) return nullptr;
    return m_vulkan->GetIDImage();
}

void* AbstractRender::GetIDImageView() const
{
    if (!m_initialized || m_currentAPI != RenderAPIType::VULKAN || !m_vulkan) return nullptr;
    return m_vulkan->GetIDImageView();
}

int AbstractRender::GetIDBufferWidth() const
{
    if (!m_initialized || m_currentAPI != RenderAPIType::VULKAN || !m_vulkan) return 0;
    return m_vulkan->GetIDBufferWidth();
}

int AbstractRender::GetIDBufferHeight() const
{
    if (!m_initialized || m_currentAPI != RenderAPIType::VULKAN || !m_vulkan) return 0;
    return m_vulkan->GetIDBufferHeight();
}

void* AbstractRender::GetPhysicalDevice() const
{
    if (!m_initialized || m_currentAPI != RenderAPIType::VULKAN || !m_vulkan) return nullptr;
    return m_vulkan->GetPhysicalDevice();
}

class PostRender* AbstractRender::GetPostRender()
{
    if (!m_initialized || m_currentAPI != RenderAPIType::VULKAN || !m_vulkan) return nullptr;
    return m_vulkan->GetPostRender();
}

void AbstractRender::InitPostRender(int width, int height)
{
    if (!m_initialized) return;
    
    if (m_currentAPI == RenderAPIType::VULKAN && m_vulkan)
    {
        m_vulkan->InitPostRender(width, height);
    }
}

void AbstractRender::BeginIDPass()
{
    if (!m_initialized || m_currentAPI != RenderAPIType::VULKAN || !m_vulkan) return;
    m_vulkan->BeginIDPass();
}

void AbstractRender::RenderModelsToID()
{
    if (!m_initialized || m_currentAPI != RenderAPIType::VULKAN || !m_vulkan) return;
    m_vulkan->RenderModelsToID();
}

void AbstractRender::EndIDPass()
{
    if (!m_initialized || m_currentAPI != RenderAPIType::VULKAN || !m_vulkan) return;
    m_vulkan->EndIDPass();
}