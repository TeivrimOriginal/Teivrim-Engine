#include "stb_image.h"
#include <GL/glew.h>
#include <iostream>
#include <string>
#include <sstream>
#include <chrono>
#include <thread>
#include <windows.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glew32.lib")
#pragma comment(lib, "assimp.lib")

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Константы
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const char* WINDOW_TITLE = "FBX Model Viewer";

// Глобальные переменные
HWND g_hwnd;
HDC g_hdc;
HGLRC g_hrc;
bool g_running = true;
glm::mat4 g_viewMatrix;
glm::mat4 g_projectionMatrix;
float g_rotationAngle = 0.0f;

// Структура вершины
struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

// Структура текстуры
struct Texture {
    GLuint id;
    std::string type;
    aiString path;
};

// Структура меша
struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;
    GLuint VAO, VBO, EBO;
    
    void SetupMesh() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        
        glBindVertexArray(VAO);
        
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
        
        // Позиции вершин
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        
        // Нормали
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
        
        // Текстурные координаты
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
        
        glBindVertexArray(0);
    }
    
    void Draw(GLuint shaderProgram) {
        unsigned int diffuseNr = 1;
        unsigned int specularNr = 1;
        
        for(unsigned int i = 0; i < textures.size(); i++) {
            glActiveTexture(GL_TEXTURE0 + i);
            
            std::string number;
            std::string name = textures[i].type;
            if(name == "texture_diffuse")
                number = std::to_string(diffuseNr++);
            else if(name == "texture_specular")
                number = std::to_string(specularNr++);
            
            glUniform1i(glGetUniformLocation(shaderProgram, (name + number).c_str()), i);
            glBindTexture(GL_TEXTURE_2D, textures[i].id);
        }
        
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        
        glActiveTexture(GL_TEXTURE0);
    }
};

// Класс модели
class Model {
public:
    Model(const char* path) {
        LoadModel(path);
    }
    
    void Draw(GLuint shaderProgram) {
        for(unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].Draw(shaderProgram);
    }
    
private:
    std::vector<Mesh> meshes;
    std::string directory;
    std::vector<Texture> textures_loaded;
    
    void LoadModel(const std::string& path) {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, 
            aiProcess_Triangulate | 
            aiProcess_FlipUVs | 
            aiProcess_GenSmoothNormals |
            aiProcess_CalcTangentSpace);
        
        if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cout << "Ошибка загрузки модели: " << importer.GetErrorString() << std::endl;
            return;
        }
        
        directory = path.substr(0, path.find_last_of("/\\"));
        ProcessNode(scene->mRootNode, scene);
    }
    
    void ProcessNode(aiNode* node, const aiScene* scene) {
        for(unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(ProcessMesh(mesh, scene));
        }
        
        for(unsigned int i = 0; i < node->mNumChildren; i++) {
            ProcessNode(node->mChildren[i], scene);
        }
    }
    
    Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene) {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        std::vector<Texture> textures;
        
        // Вершины
        for(unsigned int i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex;
            
            // Позиции
            vertex.Position = glm::vec3(
                mesh->mVertices[i].x,
                mesh->mVertices[i].y,
                mesh->mVertices[i].z
            );
            
            // Нормали
            if(mesh->HasNormals()) {
                vertex.Normal = glm::vec3(
                    mesh->mNormals[i].x,
                    mesh->mNormals[i].y,
                    mesh->mNormals[i].z
                );
            }
            
            // Текстурные координаты
            if(mesh->mTextureCoords[0]) {
                vertex.TexCoords = glm::vec2(
                    mesh->mTextureCoords[0][i].x,
                    mesh->mTextureCoords[0][i].y
                );
            } else {
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);
            }
            
            vertices.push_back(vertex);
        }
        
        // Индексы
        for(unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for(unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }
        
        // Материалы и текстуры
        if(mesh->mMaterialIndex >= 0) {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
            
            // Диффузные текстуры
            std::vector<Texture> diffuseMaps = LoadMaterialTextures(material, 
                aiTextureType_DIFFUSE, "texture_diffuse", scene);
            textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
            
            // Спекулярные текстуры
            std::vector<Texture> specularMaps = LoadMaterialTextures(material, 
                aiTextureType_SPECULAR, "texture_specular", scene);
            textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
        }
        
        Mesh resultMesh;
        resultMesh.vertices = vertices;
        resultMesh.indices = indices;
        resultMesh.textures = textures;
        resultMesh.SetupMesh();
        
        return resultMesh;
    }
    
    std::vector<Texture> LoadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName, const aiScene* scene) {
        std::vector<Texture> textures;
        
        for(unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
            aiString str;
            mat->GetTexture(type, i, &str);
            
            // Проверяем, не загружали ли уже эту текстуру
            bool skip = false;
            for(unsigned int j = 0; j < textures_loaded.size(); j++) {
                if(std::strcmp(textures_loaded[j].path.C_Str(), str.C_Str()) == 0) {
                    textures.push_back(textures_loaded[j]);
                    skip = true;
                    break;
                }
            }
            
            if(!skip) {
                // Ищем встроенную текстуру
                const aiTexture* embeddedTexture = nullptr;
                
                // Проверяем все встроенные текстуры в сцене
                for(unsigned int j = 0; j < scene->mNumTextures; j++) {
                    if(scene->mTextures[j]->mFilename.C_Str() == std::string(str.C_Str())) {
                        embeddedTexture = scene->mTextures[j];
                        break;
                    }
                }
                
                Texture texture;
                if(embeddedTexture) {
                    // ВСТРОЕННАЯ ТЕКСТУРА
                    std::cout << "Загружаем встроенную текстуру: " << str.C_Str() << std::endl;
                    texture.id = TextureFromEmbedded(embeddedTexture);
                    if(texture.id != 0) {
                        texture.type = typeName;
                        texture.path = str;
                        textures.push_back(texture);
                        textures_loaded.push_back(texture);
                    }
                } else {
                    // ВНЕШНЯЯ ТЕКСТУРА
                    std::cout << "Загружаем внешнюю текстуру: " << str.C_Str() << std::endl;
                    texture.id = TextureFromFile(str.C_Str(), directory);
                    if(texture.id != 0) {
                        texture.type = typeName;
                        texture.path = str;
                        textures.push_back(texture);
                        textures_loaded.push_back(texture);
                    }
                }
            }
        }
        return textures;
    }
    
    GLuint TextureFromEmbedded(const aiTexture* embeddedTexture) {
        GLuint textureID;
        glGenTextures(1, &textureID);
        
        int width, height, nrComponents;
        unsigned char* data = nullptr;
        
        if(embeddedTexture->mHeight == 0) {
            // Сжатый формат (JPEG, PNG и т.д.)
            std::cout << "Текстура сжатая, размер: " << embeddedTexture->mWidth << " байт" << std::endl;
            data = stbi_load_from_memory(
                reinterpret_cast<unsigned char*>(embeddedTexture->pcData),
                embeddedTexture->mWidth,
                &width,
                &height,
                &nrComponents,
                0
            );
        } else {
            // Несжатый ARGB формат
            std::cout << "Текстура несжатая, размер: " << embeddedTexture->mWidth << "x" << embeddedTexture->mHeight << std::endl;
            // Конвертируем из ARGB в RGBA
            data = new unsigned char[embeddedTexture->mWidth * embeddedTexture->mHeight * 4];
            for(unsigned int i = 0; i < embeddedTexture->mWidth * embeddedTexture->mHeight; ++i) {
                data[i*4] = embeddedTexture->pcData[i].b;
                data[i*4+1] = embeddedTexture->pcData[i].g;
                data[i*4+2] = embeddedTexture->pcData[i].r;
                data[i*4+3] = embeddedTexture->pcData[i].a;
            }
            width = embeddedTexture->mWidth;
            height = embeddedTexture->mHeight;
            nrComponents = 4;
        }
        
        if(data) {
            GLenum format;
            if(nrComponents == 1)
                format = GL_RED;
            else if(nrComponents == 3)
                format = GL_RGB;
            else if(nrComponents == 4)
                format = GL_RGBA;
            
            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
            
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            
            std::cout << "Текстура загружена: " << width << "x" << height << ", каналов: " << nrComponents << std::endl;
            
            if(embeddedTexture->mHeight == 0) {
                stbi_image_free(data);
            } else {
                delete[] data;
            }
        } else {
            std::cout << "Не удалось загрузить встроенную текстуру" << std::endl;
            glDeleteTextures(1, &textureID);
            return 0;
        }
        
        return textureID;
    }
    
    GLuint TextureFromFile(const char* path, const std::string& directory) {
        std::string filename = std::string(path);
        
        // Пробуем разные пути
        unsigned char* data = nullptr;
        int width, height, nrComponents;
        
        // 1. Пробуем как абсолютный путь
        data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
        
        if(!data && !directory.empty()) {
            // 2. Пробуем относительно директории модели
            filename = directory + '/' + std::string(path);
            data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
        }
        
        if(!data) {
            // 3. Пробуем только имя файла
            std::string simpleName = std::string(path);
            size_t pos = simpleName.find_last_of("/\\");
            if(pos != std::string::npos) {
                simpleName = simpleName.substr(pos + 1);
            }
            data = stbi_load(simpleName.c_str(), &width, &height, &nrComponents, 0);
        }
        
        if(!data) {
            std::cout << "Текстура не загружена: " << path << std::endl;
            return 0;
        }
        
        GLuint textureID;
        glGenTextures(1, &textureID);
        
        GLenum format;
        if(nrComponents == 1)
            format = GL_RED;
        else if(nrComponents == 3)
            format = GL_RGB;
        else if(nrComponents == 4)
            format = GL_RGBA;
        
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        stbi_image_free(data);
        
        return textureID;
    }
};

// Шейдеры
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    TexCoords = aTexCoords;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D texture_diffuse1;

void main()
{
    FragColor = texture(texture_diffuse1, TexCoords);
    if(FragColor.a < 0.1)
        discard;
}
)";

// Функции для работы с OpenGL
GLuint CompileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if(!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cout << "Ошибка компиляции шейдера: " << infoLog << std::endl;
        return 0;
    }
    
    return shader;
}

GLuint CreateShaderProgram() {
    GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    GLint success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if(!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "Ошибка линковки шейдеров: " << infoLog << std::endl;
        return 0;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return shaderProgram;
}

// Функции Win32
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch(uMsg) {
        case WM_CLOSE:
            g_running = false;
            DestroyWindow(hwnd);
            return 0;
            
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
            
        case WM_KEYDOWN:
            if(wParam == VK_ESCAPE)
                g_running = false;
            return 0;
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

bool InitWindow(HINSTANCE hInstance) {
    // Регистрация класса окна
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "FBXViewerClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    
    if(!RegisterClass(&wc)) return false;
    
    // Создание окна
    g_hwnd = CreateWindowEx(
        0,
        "FBXViewerClass",
        WINDOW_TITLE,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        NULL,
        NULL,
        hInstance,
        NULL
    );
    
    if(!g_hwnd) return false;
    
    // Получение контекста устройства
    g_hdc = GetDC(g_hwnd);
    
    // Настройка формата пикселей
    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA,
        32,
        0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0,
        24, // Глубина буфера цвета
        8,  // Альфа-буфер
        0,
        0, 0, 0, 0
    };
    
    int pixelFormat = ChoosePixelFormat(g_hdc, &pfd);
    SetPixelFormat(g_hdc, pixelFormat, &pfd);
    
    // Создание контекста OpenGL
    g_hrc = wglCreateContext(g_hdc);
    wglMakeCurrent(g_hdc, g_hrc);
    
    // Инициализация GLEW
    glewExperimental = GL_TRUE;
    if(glewInit() != GLEW_OK) {
        std::cout << "Ошибка инициализации GLEW!" << std::endl;
        return false;
    }
    
    // Настройка OpenGL
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    
    ShowWindow(g_hwnd, SW_SHOW);
    UpdateWindow(g_hwnd);
    
    return true;
}

void Cleanup() {
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(g_hrc);
    ReleaseDC(g_hwnd, g_hdc);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Инициализация окна и OpenGL
    if(!InitWindow(hInstance)) {
        MessageBox(NULL, "Ошибка инициализации окна!", "Ошибка", MB_OK | MB_ICONERROR);
        return -1;
    }
    
    // Создание шейдерной программы
    GLuint shaderProgram = CreateShaderProgram();
    if(!shaderProgram) {
        MessageBox(NULL, "Ошибка создания шейдеров!", "Ошибка", MB_OK | MB_ICONERROR);
        return -1;
    }
    
    // Загрузка модели FBX
    // Замените "model.fbx" на путь к вашему FBX файлу
    std::string modelPath = "model.fbx";  // Измените на ваш путь
    
    // Пробуем загрузить из текущей директории
    Model* ourModel = nullptr;
    try {
        std::cout << "Пытаемся загрузить модель: " << modelPath << std::endl;
        ourModel = new Model(modelPath.c_str());
        std::cout << "Модель успешно загружена!" << std::endl;
    } catch(const std::exception& e) {
        std::cout << "Ошибка загрузки модели: " << e.what() << std::endl;
        MessageBox(NULL, "Ошибка загрузки модели!", "Ошибка", MB_OK | MB_ICONERROR);
        return -1;
    }
    
    // Настройка матриц
    g_projectionMatrix = glm::perspective(
        glm::radians(45.0f),
        (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT,
        0.1f,
        100.0f
    );
    
    g_viewMatrix = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 5.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    
    // Основной цикл
    MSG msg = {};
    auto lastTime = std::chrono::high_resolution_clock::now();
    
    while(g_running) {
        // Обработка сообщений Windows
        while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        // Расчет дельты времени
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;
        
        // Обновление
        g_rotationAngle += 50.0f * deltaTime;
        
        // Очистка буферов
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Использование шейдерной программы
        glUseProgram(shaderProgram);
        
        // Установка uniform-переменных
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(g_rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.01f)); // Масштаб для FBX моделей
        
        GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
        GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
        GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
        
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(g_viewMatrix));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(g_projectionMatrix));
        
        // Отрисовка модели
        if(ourModel) {
            ourModel->Draw(shaderProgram);
        }
        
        // Переключение буферов
        SwapBuffers(g_hdc);
        
        // Небольшая пауза для снижения нагрузки на CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    delete ourModel;
    Cleanup();
    return 0;
}