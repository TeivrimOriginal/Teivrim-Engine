#ifndef ABSTRACT_RENDER_H
#define ABSTRACT_RENDER_H

#include <string>
#include <vector>
#include <glm/glm.hpp>

class InitialWin32;
class InterfaceManager;
class Vulkan;
struct VulkanTexture;

enum class RenderAPIType {
    OPENGL,
    VULKAN
};

class AbstractRender {
public:
    static AbstractRender& Instance();
    
    bool Initialize(RenderAPIType api, InitialWin32* window, int width, int height);
    void Shutdown();
    
    void BeginFrame();
    void EndFrame();
    void Present();
    
    void Setup2D(int width, int height);
    
    void DrawBackground(float x1, float y1, float x2, float y2, float r, float g, float b);
    void DrawQuad(float x1, float y1, float x2, float y2, float r, float g, float b);
    void DrawText(int x, int y, const std::string& text, float r, float g, float b);
    void DrawTextCentered(int x, int y, int w, int h, const std::string& text, float r, float g, float b);
    void DrawImage(float x1, float y1, float x2, float y2, VulkanTexture* texture);
    void DrawImageUV(float x1, float y1, float x2, float y2, VulkanTexture* texture, float u1, float v1, float u2, float v2);
    
    VulkanTexture* LoadUIImage(const std::string& filepath);
    VulkanTexture* LoadUIImageFromData(unsigned char* data, int width, int height, int channels);
    void FreeUIImage(VulkanTexture* texture);
    
    int AddModel(const std::string& name, const std::vector<class StandardMesh>& meshes);
    void RemoveModel(const std::string& name);
    void ClearModels();
    void RenderAllModels();
    void RenderModel(const std::string& name);
    void SetModelTransform(const std::string& name, const glm::mat4& transform);
    
    void SetViewMatrix(const glm::mat4& view);
    void SetProjectionMatrix(const glm::mat4& proj);
    
    void RecreateSwapchain();
    
    void* GetDevice() const;
    void* GetRenderPass() const;
    void* GetCurrentCommandBuffer() const;
    void* BeginSingleTimeCommands();
    void EndSingleTimeCommands(void* cmdBuffer);
    uint32_t FindMemoryType(uint32_t typeFilter, uint32_t properties);
    
    void SetViewportClip(int x, int y, int w, int h);
    void DisableViewportClip();
    bool IsViewportClippingEnabled() const;
    
    void FlushBackground();
    
    void RenderBackground();
    void RenderScene();
    void RenderOverlay();
    
    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix() const;
    
    void RenderGrid(const glm::mat4& viewMatrix, const glm::mat4& projMatrix);
    void SetGridEnabled(bool enabled);
    void SetGridSpacing(float spacing);
    void SetGridFadeDistance(float distance);
    void SetGridLineColor(float r, float g, float b);
    void SetGridCenterLineColor(float r, float g, float b);
    bool IsGridEnabled() const;
    float GetGridSpacing() const;
    
    void* GetIDImage() const;
    void* GetIDImageView() const;
    int GetIDBufferWidth() const;
    int GetIDBufferHeight() const;
    void* GetPhysicalDevice() const;
    
    class PostRender* GetPostRender();
    
    void InitPostRender(int width, int height);
    void BeginIDPass();
    void RenderModelsToID();
    void EndIDPass();
    
    bool IsInitialized() const { return m_initialized; }
    RenderAPIType GetCurrentAPI() const { return m_currentAPI; }
    
private:
    AbstractRender();
    ~AbstractRender();
    
    AbstractRender(const AbstractRender&) = delete;
    AbstractRender& operator=(const AbstractRender&) = delete;
    
    RenderAPIType m_currentAPI;
    bool m_initialized;
    
    class Vulkan* m_vulkan;
    class RendererW* m_opengl;
    int m_width;
    int m_height;
    HWND m_hwnd;
};

#endif