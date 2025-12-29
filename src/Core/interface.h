#ifndef INTERFACE_H
#define INTERFACE_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>

class Interface {
public:
    Interface();
    ~Interface();
    
    void initialize(GLFWwindow* window);
    void render();
    void cleanup();
    
    void setWindowSize(int width, int height);
    
private:
    void createBorderVAO();
    
    GLuint shaderProgram;
    GLuint borderVAO;
    GLuint borderVBO;
    
    int screenWidth;
    int screenHeight;
    
    GLFWwindow* window;
    
    // Шейдеры для интерфейса
    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec2 aPos;
        
        void main() {
            gl_Position = vec4(aPos, 0.0, 1.0);
        }
    )";
    
    const char* fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;
        uniform vec3 borderColor;
        
        void main() {
            FragColor = vec4(borderColor, 1.0);
        }
    )";
};

#endif