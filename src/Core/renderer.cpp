#include "renderer.h"
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoords = aTexCoords;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 objectColor;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;

void main() {
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * lightColor;
    
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;
    
    vec3 result = (ambient + diffuse + specular) * objectColor;
    FragColor = vec4(result, 1.0);
}
)";

Renderer::Renderer() 
    : window(nullptr), 
      camera(glm::vec3(0.0f, 0.0f, 5.0f)),
      lastX(400.0f), lastY(300.0f), firstMouse(true),
      deltaTime(0.0f), lastFrame(0.0f),
      animateModel(true),
      sprintEnabled(false) {}

Renderer::~Renderer() {
    cleanup();
}

bool Renderer::initialize() {
    if (!glfwInit()) return false;
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    window = glfwCreateWindow(800, 600, "3D Model Viewer", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return false;
    }
    
    glfwMakeContextCurrent(window);
    
    glfwSetWindowUserPointer(window, this);
    glfwSetCursorPosCallback(window, [](GLFWwindow* window, double xpos, double ypos) {
        Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
        renderer->mouseCallback(xpos, ypos);
    });
    glfwSetScrollCallback(window, [](GLFWwindow* window, double xoffset, double yoffset) {
        Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
        renderer->scrollCallback(xoffset, yoffset);
    });
    
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cout << "Failed to initialize GLEW" << std::endl;
        return false;
    }
    
    glEnable(GL_DEPTH_TEST);
    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
    
    std::cout << "\n=== CONTROLS ===" << std::endl;
    std::cout << "WASD - Move camera" << std::endl;
    std::cout << "Space - Move up" << std::endl;
    std::cout << "Ctrl - Move down" << std::endl;
    std::cout << "Shift - Sprint (hold to speed up)" << std::endl;
    std::cout << "Mouse - Look around" << std::endl;
    std::cout << "Mouse Wheel - Zoom" << std::endl;
    std::cout << "R - Toggle model rotation" << std::endl;
    std::cout << "F - Toggle sprint mode" << std::endl;
    std::cout << "ESC - Exit" << std::endl;
    std::cout << "================\n" << std::endl;
    
    return true;
}

void Renderer::cleanup() {
    for (auto vao : VAOs) glDeleteVertexArrays(1, &vao);
    for (auto vbo : VBOs) glDeleteBuffers(1, &vbo);
    for (auto ebo : EBOs) glDeleteBuffers(1, &ebo);
    
    VAOs.clear();
    VBOs.clear();
    EBOs.clear();
    
    if (window) {
        glfwDestroyWindow(window);
        window = nullptr;
    }
    glfwTerminate();
}

void Renderer::beginFrame() {
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
    
    processInput(deltaTime);
    
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::endFrame() {
    glfwSwapBuffers(window);
    glfwPollEvents();
}

void Renderer::processInput(float deltaTime) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    
    float baseSpeed = 5.0f;
    float currentSpeed = baseSpeed;
    bool shiftPressed = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
    
    if (shiftPressed) {
        currentSpeed = baseSpeed * 3.0f;
        if (!sprintEnabled) {
            std::cout << "SPRINT ENABLED (3x speed)" << std::endl;
            sprintEnabled = true;
        }
    } else {
        if (sprintEnabled) {
            std::cout << "SPRINT DISABLED" << std::endl;
            sprintEnabled = false;
        }
    }
    
    camera.SetMovementSpeed(currentSpeed);
    
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        camera.ProcessKeyboard(FORWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        camera.ProcessKeyboard(LEFT, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        camera.ProcessKeyboard(RIGHT, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        camera.ProcessKeyboard(UP, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
        camera.ProcessKeyboard(DOWN, deltaTime);
    }
    
    static bool rKeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS && !rKeyPressed) {
        animateModel = !animateModel;
        std::cout << "Model rotation: " << (animateModel ? "ENABLED" : "DISABLED") << std::endl;
        rKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_RELEASE) {
        rKeyPressed = false;
    }
    
    static bool fKeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS && !fKeyPressed) {
        sprintEnabled = !sprintEnabled;
        std::cout << "Sprint mode: " << (sprintEnabled ? "ALWAYS ON" : "HOLD SHIFT") << std::endl;
        fKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_RELEASE) {
        fKeyPressed = false;
    }
    
    if (sprintEnabled && !shiftPressed) {
        camera.SetMovementSpeed(baseSpeed * 3.0f);
    }
}

void Renderer::mouseCallback(double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    
    lastX = xpos;
    lastY = ypos;
    
    camera.ProcessMouseMovement(xoffset, yoffset);
}

void Renderer::scrollCallback(double xoffset, double yoffset) {
    camera.ProcessMouseScroll(yoffset);
}

void Renderer::renderModel(const ModelParser& model, GLuint shaderProgram) {
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
    
    const auto& meshes = model.getMeshes();
    std::vector<glm::vec3> colors = {
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
                  << " Animation: " << (animateModel ? "ON" : "OFF")
                  << " Sprint: " << (sprintEnabled ? "ON" : "OFF")
                  << std::endl;
        
        lastInfoTime = currentTime;
    }
    
    for (size_t i = 0; i < meshes.size(); i++) {
        glm::vec3 color = colors[i % colors.size()];
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
                 mesh.vertexBuffer.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(unsigned int),
                 mesh.indices.data(), GL_STATIC_DRAW);
    
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
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cout << "Shader compilation failed: " << infoLog << std::endl;
    }
    return shader;
}

GLuint initShaders() {
    GLuint vertexShader = compileShader(vertexShaderSource, GL_VERTEX_SHADER);
    GLuint fragmentShader = compileShader(fragmentShaderSource, GL_FRAGMENT_SHADER);
    
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    
    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cout << "Program linking failed: " << infoLog << std::endl;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return program;
}