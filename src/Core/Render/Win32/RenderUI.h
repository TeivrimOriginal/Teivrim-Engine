#ifndef RENDERUI_H
#define RENDERUI_H

#include <windows.h>
#include <GL/glew.h>
#include <string>
#include <vector>
#include <glm/glm.hpp>

enum class RenderAPIType { OPENGL, VULKAN };

class Vulkan;
struct VulkanTexture;

class RenderUI {
public:
    RenderUI(RenderAPIType type = RenderAPIType::OPENGL);
    ~RenderUI();

    void initialize(HWND hwnd, int width, int height);
    void setVulkan(Vulkan* vk);
    
    void setup2D(int width, int height);
    
    void drawQuad(float x1, float y1, float x2, float y2, float r, float g, float b);
    void drawText(int x, int y, const std::string& text, float r, float g, float b);
    void drawTextCentered(int x, int y, int w, int h, const std::string& text, float r, float g, float b);
    void drawImage(int x, int y, int w, int h, void* texture);
    
    GLuint loadTextureFromFile(const std::string& path);
    
    void saveState(GLint& prog, GLint vp[4], GLboolean& dt);
    void restoreState(GLint prog, GLint vp[4], GLboolean dt);
    void restoreMatrices();
    
private:
    RenderAPIType apiType;
    Vulkan* vulkan;
    
    int screenWidth, screenHeight;
    
    GLuint shaderProgram;
    GLuint vao, vbo;
};

#endif