#ifndef RENDERER_H
#define RENDERER_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "parser.h"
#include "camera.h"

class Renderer {
public:
    Renderer();
    ~Renderer();
    
    bool initialize();
    void cleanup();
    bool shouldClose() const { return glfwWindowShouldClose(window); }
    void beginFrame();
    void endFrame();
    void renderModel(const ModelParser& model, GLuint shaderProgram);
    
    void processInput(float deltaTime);
    void mouseCallback(double xpos, double ypos);
    void scrollCallback(double xoffset, double yoffset);
    
    GLFWwindow* getWindow() const { return window; }
    Camera& getCamera() { return camera; }
    
    void setAnimateModel(bool animate) { animateModel = animate; }
    bool getAnimateModel() const { return animateModel; }
    void toggleAnimateModel() { animateModel = !animateModel; }
    
    void setSprintEnabled(bool enabled) { sprintEnabled = enabled; }
    bool getSprintEnabled() const { return sprintEnabled; }
    void toggleSprint() { sprintEnabled = !sprintEnabled; }

private:
    void renderStandardMesh(const StandardMesh& mesh, GLuint shaderProgram);
    GLuint createMeshBuffers(const StandardMesh& mesh);
    
    GLFWwindow* window;
    Camera camera;
    
    float lastX, lastY;
    bool firstMouse;
    
    float deltaTime;
    float lastFrame;
    
    bool animateModel;
    bool sprintEnabled;
    
    std::vector<GLuint> VAOs;
    std::vector<GLuint> VBOs;
    std::vector<GLuint> EBOs;
};

GLuint compileShader(const char* source, GLenum type);
GLuint initShaders();

#endif