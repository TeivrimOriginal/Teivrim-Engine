#include "rendererw.h"
#include <iostream>
#include <windows.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
using namespace std;

// Шейдеры (оставляем как есть)
const char* RendererW::vertexShaderSource = 
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

const char* RendererW::fragmentShaderSource = 
"#version 330 core\n"
"out vec4 FragColor;\n"
"\n"
"in vec3 FragPos;\n"
"in vec3 Normal;\n"
"in vec2 TexCoords;\n"
"\n"
"uniform sampler2D texture_diffuse1;\n"
"uniform sampler2D texture_specular1;\n"
"uniform vec3 objectColor;\n"
"uniform vec3 lightPos;\n"
"uniform vec3 lightColor;\n"
"uniform vec3 viewPos;\n"
"uniform bool useTexture;\n"
"\n"
"void main() {\n"
"    vec4 texColor = texture(texture_diffuse1, TexCoords);\n"
"    \n"
"    if(texColor.a < 0.1) {\n"
"        discard;\n"
"    }\n"
"    \n"
"    vec3 color = useTexture ? texColor.rgb : objectColor;\n"
"    \n"
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
"    vec3 result = (ambient + diffuse + specular) * color;\n"
"    FragColor = vec4(result, 1.0);\n"
"}\n";

RendererW::RendererW() 
    : window(nullptr), 
      animateModel(true),
      cachedShaderProgram(0),
      cachedModelLoc(-1),
      cachedViewLoc(-1),
      cachedProjectionLoc(-1),
      cachedLightColorLoc(-1),
      cachedLightPosLoc(-1),
      cachedViewPosLoc(-1),
      cachedUseTextureLoc(-1),
      cachedObjectColorLoc(-1) {}

RendererW::~RendererW() {
    cleanup();
}

bool RendererW::initialize(InitialWin32* win) {
    window = win;
    if (!window) {
        cout << "Invalid window passed to RendererW" << endl;
        return false;
    }
    
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        cout << "Failed to initialize GLEW" << endl;
        return false;
    }
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    cout << "OpenGL version: " << glGetString(GL_VERSION) << endl;
    
    return true;
}

void RendererW::cleanup() {
    // Очищаем старые буферы
    for (size_t i = 0; i < VAOs.size(); i++) {
        glDeleteVertexArrays(1, &VAOs[i]);
    }
    for (size_t i = 0; i < VBOs.size(); i++) {
        glDeleteBuffers(1, &VBOs[i]);
    }
    for (size_t i = 0; i < EBOs.size(); i++) {
        glDeleteBuffers(1, &EBOs[i]);
    }
    
    // Очищаем оптимизированные буферы
    for (size_t i = 0; i < meshVAOs.size(); i++) {
        glDeleteVertexArrays(1, &meshVAOs[i].vao);
        glDeleteBuffers(1, &meshVAOs[i].vbo);
        glDeleteBuffers(1, &meshVAOs[i].ebo);
    }
    
    VAOs.clear();
    VBOs.clear();
    EBOs.clear();
    meshVAOs.clear();
}

void RendererW::beginFrame() {
}

void RendererW::endFrame() {
    window->swapBuffers();
    window->pollEvents();
}

// РЕАЛИЗАЦИЯ МЕТОДА OPTIMIZE
void RendererW::optimize(const ModelParser& model, GLuint shaderProgram) {
    // Очищаем старые буферы
    for (size_t i = 0; i < meshVAOs.size(); i++) {
        glDeleteVertexArrays(1, &meshVAOs[i].vao);
        glDeleteBuffers(1, &meshVAOs[i].vbo);
        glDeleteBuffers(1, &meshVAOs[i].ebo);
    }
    meshVAOs.clear();
    
    const vector<StandardMesh>& meshes = model.getMeshes();
    
    for (size_t i = 0; i < meshes.size(); i++) {
        const StandardMesh& mesh = meshes[i];
        MeshBuffers buffers;
        
        glGenVertexArrays(1, &buffers.vao);
        glGenBuffers(1, &buffers.vbo);
        glGenBuffers(1, &buffers.ebo);
        
        glBindVertexArray(buffers.vao);
        
        glBindBuffer(GL_ARRAY_BUFFER, buffers.vbo);
        glBufferData(GL_ARRAY_BUFFER, mesh.vertexBuffer.size() * sizeof(float), 
                    &mesh.vertexBuffer[0], GL_STATIC_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers.ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(unsigned int),
                    &mesh.indices[0], GL_STATIC_DRAW);
        
        // Vertex attributes
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
        
        glBindVertexArray(0);
        
        buffers.indexCount = mesh.indices.size();
        buffers.textures = mesh.textures;
        meshVAOs.push_back(buffers);
        
        // Сохраняем в общие списки для cleanup
        VAOs.push_back(buffers.vao);
        VBOs.push_back(buffers.vbo);
        EBOs.push_back(buffers.ebo);
    }
    
    // Кэшируем uniform locations
    cachedShaderProgram = shaderProgram;
    cachedModelLoc = glGetUniformLocation(shaderProgram, "model");
    cachedViewLoc = glGetUniformLocation(shaderProgram, "view");
    cachedProjectionLoc = glGetUniformLocation(shaderProgram, "projection");
    cachedLightColorLoc = glGetUniformLocation(shaderProgram, "lightColor");
    cachedLightPosLoc = glGetUniformLocation(shaderProgram, "lightPos");
    cachedViewPosLoc = glGetUniformLocation(shaderProgram, "viewPos");
    cachedUseTextureLoc = glGetUniformLocation(shaderProgram, "useTexture");
    cachedObjectColorLoc = glGetUniformLocation(shaderProgram, "objectColor");
    
    cout << "Model optimized with " << meshVAOs.size() << " meshes" << endl;
}

void RendererW::renderModel(const ModelParser& model, GLuint shaderProgram, Camera& camera) {
    glUseProgram(shaderProgram);
    
    // Автоматическая оптимизация при первом рендере или смене шейдера
    if (meshVAOs.empty() || cachedShaderProgram != shaderProgram) {
        optimize(model, shaderProgram);
    }
    
    // Матрица модели
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 0.0f, 0.0f)); 
    modelMatrix = glm::scale(modelMatrix, glm::vec3(0.01f));
    
    if (animateModel) {
        double currentTime = GetTickCount() / 1000.0;
        modelMatrix = glm::rotate(modelMatrix, (float)currentTime * 0.5f, glm::vec3(0.0f, 1.0f, 0.0f));
    }
    
    glm::mat4 view = camera.GetViewMatrix();
    
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    float aspectRatio = (float)viewport[2] / (float)viewport[3];
    
    glm::mat4 projection = glm::perspective(
        glm::radians(camera.GetZoom()),
        aspectRatio,
        0.1f,
        100.0f
    );
    
    // Устанавливаем uniform-ы
    if (cachedModelLoc != -1)
        glUniformMatrix4fv(cachedModelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
    if (cachedViewLoc != -1)
        glUniformMatrix4fv(cachedViewLoc, 1, GL_FALSE, glm::value_ptr(view));
    if (cachedProjectionLoc != -1)
        glUniformMatrix4fv(cachedProjectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
    
    if (cachedLightColorLoc != -1)
        glUniform3f(cachedLightColorLoc, 1.0f, 1.0f, 1.0f);
    if (cachedLightPosLoc != -1)
        glUniform3f(cachedLightPosLoc, 2.0f, 5.0f, 2.0f);
    if (cachedViewPosLoc != -1)
        glUniform3f(cachedViewPosLoc, 
                    camera.GetPosition().x, camera.GetPosition().y, camera.GetPosition().z);
    
    // Рендерим меши
    for (size_t i = 0; i < meshVAOs.size(); i++) {
        const MeshBuffers& buffers = meshVAOs[i];
        
        bool useTexture = !buffers.textures.empty();
        if (cachedUseTextureLoc != -1)
            glUniform1i(cachedUseTextureLoc, useTexture);
        
        if (!useTexture) {
            glm::vec3 colors[] = {
                glm::vec3(0.8f, 0.3f, 0.2f),
                glm::vec3(0.2f, 0.8f, 0.3f),
                glm::vec3(0.3f, 0.2f, 0.8f),
                glm::vec3(0.8f, 0.8f, 0.2f),
                glm::vec3(0.8f, 0.2f, 0.8f),
                glm::vec3(0.2f, 0.8f, 0.8f)
            };
            glm::vec3 color = colors[i % 6];
            if (cachedObjectColorLoc != -1)
                glUniform3f(cachedObjectColorLoc, color.r, color.g, color.b);
        } else {
            unsigned int diffuseNr = 1;
            unsigned int specularNr = 1;
            
            for(unsigned int j = 0; j < buffers.textures.size(); j++) {
                glActiveTexture(GL_TEXTURE0 + j);
                
                string number;
                string name = buffers.textures[j].type;
                if(name == "texture_diffuse")
                    number = to_string(diffuseNr++);
                else if(name == "texture_specular")
                    number = to_string(specularNr++);
                
                glUniform1i(glGetUniformLocation(shaderProgram, (name + number).c_str()), j);
                glBindTexture(GL_TEXTURE_2D, buffers.textures[j].id);
            }
            glActiveTexture(GL_TEXTURE0);
        }
        
        glBindVertexArray(buffers.vao);
        glDrawElements(GL_TRIANGLES, buffers.indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
}

// Остальные методы без изменений
GLuint RendererW::createMeshBuffers(const StandardMesh& mesh) {
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

void RendererW::renderStandardMesh(const StandardMesh& mesh, GLuint shaderProgram) {
    GLuint VAO = createMeshBuffers(mesh);
    
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

GLuint RendererW::compileShader(const char* source, GLenum type) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, 0);
    glCompileShader(shader);
    
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, 0, infoLog);
        cout << "Shader compilation failed: " << infoLog << endl;
    }
    return shader;
}

GLuint RendererW::initShaders() {
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
        glGetProgramInfoLog(program, 512, 0, infoLog);
        cout << "Program linking failed: " << infoLog << endl;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return program;
}
