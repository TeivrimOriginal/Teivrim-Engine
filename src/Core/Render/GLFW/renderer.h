#ifndef RENDERER_H
#define RENDERER_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>
#include "../Parser/parser.h"
#include "../../../Control/camera.h"

class Renderer {
public:
    Renderer();
    ~Renderer();
    
    bool initialize(GLFWwindow* win);
    void cleanup();
    
    void beginFrame();
    void endFrame();
    
    void renderModel(const ModelParser& model, GLuint shaderProgram, Camera& camera);
    
    void setAnimateModel(bool animate) { animateModel = animate; }
    GLuint createMeshBuffers(const StandardMesh& mesh);
    void renderStandardMesh(const StandardMesh& mesh, GLuint shaderProgram);
    
    GLFWwindow* getWindow() const { return window; }
    
private:
    GLFWwindow* window;
    std::vector<GLuint> VAOs;
    std::vector<GLuint> VBOs;
    std::vector<GLuint> EBOs;
    
    bool animateModel;
};

GLuint compileShader(const char* source, GLenum type);
GLuint initShaders();

#endif