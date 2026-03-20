#include "renderer.h"
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
using namespace std;
const char* vertexShaderSource = 
"#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"layout (location = 1) in vec3 aNormal;\n"
"layout (location = 2) in vec2 aTexCoords;\n"
"\n"
"out vec3 FragPos;\n"
"out vec3 Normal;\n"
"out vec2 TexCoords;\n"
"\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"
"\n"
"void main() {\n"
"    FragPos = vec3(model * vec4(aPos, 1.0));\n"
"    Normal = mat3(transpose(inverse(model))) * aNormal;\n"
"    TexCoords = aTexCoords;\n"
"    gl_Position = projection * view * vec4(FragPos, 1.0);\n"
"}\n";

const char* fragmentShaderSource = 
"#version 330 core\n"
"out vec4 FragColor;\n"
"\n"
"in vec3 FragPos;\n"
"in vec3 Normal;\n"
"in vec2 TexCoords;\n"
"\n"
"uniform vec3 objectColor;\n"
"uniform vec3 lightPos;\n"
"uniform vec3 lightColor;\n"
"uniform vec3 viewPos;\n"
"\n"
"void main() {\n"
"    float ambientStrength = 0.3;\n"
"    vec3 ambient = ambientStrength * lightColor;\n"
"    \n"
"    vec3 norm = normalize(Normal);\n"
"    vec3 lightDir = normalize(lightPos - FragPos);\n"
"    float diff = max(dot(norm, lightDir), 0.0);\n"
"    vec3 diffuse = diff * lightColor;\n"
"    \n"
"    float specularStrength = 0.5;\n"
"    vec3 viewDir = normalize(viewPos - FragPos);\n"
"    vec3 reflectDir = reflect(-lightDir, norm);\n"
"    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);\n"
"    vec3 specular = specularStrength * spec * lightColor;\n"
"    \n"
"    vec3 result = (ambient + diffuse + specular) * objectColor;\n"
"    FragColor = vec4(result, 1.0);\n"
"}\n";

Renderer::Renderer() 
    : window(0), 
      animateModel(true) {}

Renderer::~Renderer() {
    cleanup();
}

bool Renderer::initialize(GLFWwindow* win) {
    window = win;
    if (!window) {
        std::cout << "Invalid window passed to Renderer" << std::endl;
        return false;
    }
    
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cout << "Failed to initialize GLEW" << std::endl;
        return false;
    }
    
    glEnable(GL_DEPTH_TEST);
    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
    
    return true;
}

void Renderer::cleanup() {
    for (size_t i = 0; i < VAOs.size(); i++) {
        glDeleteVertexArrays(1, &VAOs[i]);
    }
    for (size_t i = 0; i < VBOs.size(); i++) {
        glDeleteBuffers(1, &VBOs[i]);
    }
    for (size_t i = 0; i < EBOs.size(); i++) {
        glDeleteBuffers(1, &EBOs[i]);
    }
    
    VAOs.clear();
    VBOs.clear();
    EBOs.clear();
}

void Renderer::beginFrame() {
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::endFrame() {
    glfwSwapBuffers(window);
    glfwPollEvents();
}
//// SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSСНИЗУ ХУЕСОС
//// SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSСНИЗУ ХУЕСОС
//// SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSСНИЗУ ХУЕСОС
//// SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSСНИЗУ ХУЕСОС
//// SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSСНИЗУ ХУЕСОС
//// SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSСНИЗУ ХУЕСОС
//// SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSСНИЗУ ХУЕСОС
//// SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS СНИЗУ ХУЕСОС
//// SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSСНИЗУ ХУЕСОС
void Renderer::renderModel(const ModelParser& model, GLuint shaderProgram, Camera& camera) { 
    glUseProgram(shaderProgram);
    
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 0.0f, 0.0f)); 
    modelMatrix = glm::scale(modelMatrix, glm::vec3(1.0f, 1.0f, 1.0f));
    
    if (animateModel) {
        modelMatrix = glm::rotate(modelMatrix, (float)glfwGetTime() * 0.5f, glm::vec3(0.0f, 1.0f, 0.0f));
    }
    
    glm::mat4 view = camera.GetViewMatrix();
    
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    float aspectRatio = (float)width / (float)height;
    
    glm::mat4 projection = glm::perspective(
        glm::radians(camera.GetZoom()),
        aspectRatio,
        0.0001f,
        1000000.0f
    );
    
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    
    glUniform3f(glGetUniformLocation(shaderProgram, "lightColor"), 1.0f, 1.0f, 1.0f);
    glUniform3f(glGetUniformLocation(shaderProgram, "lightPos"), 2.0f, 5.0f, 2.0f);
    glUniform3f(glGetUniformLocation(shaderProgram, "viewPos"), 
                camera.GetPosition().x, camera.GetPosition().y, camera.GetPosition().z);
    
    const std::vector<StandardMesh>& meshes = model.getMeshes();
    glm::vec3 colors[] = {
        glm::vec3(0.8f, 0.3f, 0.2f),
        glm::vec3(0.2f, 0.8f, 0.3f),
        glm::vec3(0.3f, 0.2f, 0.8f),
        glm::vec3(0.8f, 0.8f, 0.2f),
        glm::vec3(0.8f, 0.2f, 0.8f),
        glm::vec3(0.2f, 0.8f, 0.8f)
    };
    
    static int frameCounter = 0;
    static float lastInfoTime = 0.0f;
    frameCounter++;
    
    float currentTime = glfwGetTime();
    if (currentTime - lastInfoTime > 2.0f) {
        glm::vec3 camPos = camera.GetPosition();
        float distanceToOrigin = glm::length(camPos);
        
        std::cout << "Camera: Pos(" << camPos.x << ", " << camPos.y << ", " << camPos.z << ")" 
                  << " Distance: " << distanceToOrigin
                  << " Zoom: " << camera.GetZoom()
                  << " Animation: " << (animateModel ? "ON" : "OFF") << std::endl;
        
        lastInfoTime = currentTime;
    }
    
    for (size_t i = 0; i < meshes.size(); i++) {
        glm::vec3 color = colors[i % 6];
        glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), color.r, color.g, color.b);
        renderStandardMesh(meshes[i], shaderProgram);
    }
}

GLuint Renderer::createMeshBuffers(const StandardMesh& mesh) {
    GLuint VAO, VBO, EBO;
    
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, mesh.vertexBuffer.size() * sizeof(float), 
                 &mesh.vertexBuffer[0], GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(unsigned int),
                 &mesh.indices[0], GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);
    
    VAOs.push_back(VAO);
    VBOs.push_back(VBO);
    EBOs.push_back(EBO);
    
    return VAO;
}

void Renderer::renderStandardMesh(const StandardMesh& mesh, GLuint shaderProgram) {
    GLuint VAO = createMeshBuffers(mesh);
    
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

GLuint compileShader(const char* source, GLenum type) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, 0);
    glCompileShader(shader);
    
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, 0, infoLog);
        std::cout << "Shader compilation failed: " << infoLog << std::endl;
    }
    return shader;
}

GLuint initShaders() {
    
    cout << "hui1" << endl;
    GLuint vertexShader = compileShader(vertexShaderSource, GL_VERTEX_SHADER);
    GLuint fragmentShader = compileShader(fragmentShaderSource, GL_FRAGMENT_SHADER);
    
    cout << "hui1" << endl;
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    
    cout << "hui2" << endl;
    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, 0, infoLog);
        std::cout << "Program linking failed: " << infoLog << std::endl;
    }
    
    cout << "hui3" << endl;
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return program;
}