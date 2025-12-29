#include <iostream>
#include <vector>
#include <cmath>
#include "glew.h"
#include "glfw3.h"

const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
void main()
{
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
void main()
{
    FragColor = vec4(1.0, 1.0, 1.0, 1.0); // БЕЛЫЙ ЦВЕТ
}
)";

// КЛАСС ДЛЯ РАБОТЫ С ВЕРШИНАМИ И РЁБРАМИ
class ShapeBuilder {
private:
    std::vector<float> vertices;
    std::vector<unsigned int> edges;
    
public:
    // ДОБАВЛЕНИЕ ВЕРШИНЫ ПО КООРДИНАТАМ X, Y
    void addVertex(float x, float y) {
        vertices.push_back(x);
        vertices.push_back(y);
    }
    
    // ДОБАВЛЕНИЕ РЁБРА МЕЖДУ ДВУМЯ ВЕРШИНАМИ (по индексам)
    void addEdge(unsigned int startIdx, unsigned int endIdx) {
        if (startIdx < vertices.size()/2 && endIdx < vertices.size()/2) {
            edges.push_back(startIdx);
            edges.push_back(endIdx);
        }
    }
    
    // СОЗДАНИЕ КВАДРАТА ПО ЦЕНТРУ И РАЗМЕРУ
    void createSquare(float centerX, float centerY, float size) {
        vertices.clear();
        edges.clear();
        
        float halfSize = size / 2.0f;
        
        // 4 ВЕРШИНЫ КВАДРАТА
        addVertex(centerX - halfSize, centerY + halfSize); // левый верхний (0)
        addVertex(centerX - halfSize, centerY - halfSize); // левый нижний (1)
        addVertex(centerX + halfSize, centerY - halfSize); // правый нижний (2)
        addVertex(centerX + halfSize, centerY + halfSize); // правый верхний (3)
        
        // 4 РЁБРА КВАДРАТА
        addEdge(0, 1); // левое
        addEdge(1, 2); // нижнее
        addEdge(2, 3); // правое
        addEdge(3, 0); // верхнее
    }
    
    // ПОЛУЧЕНИЕ ДАННЫХ ВЕРШИН
    const float* getVertices() const {
        return vertices.data();
    }
    
    // ПОЛУЧЕНИЕ КОЛИЧЕСТВА ВЕРШИН (в координатах)
    int getVertexCount() const {
        return vertices.size() / 2;
    }
    
    // ПОЛУЧЕНИЕ ДАННЫХ РЁБЕР
    const unsigned int* getEdges() const {
        return edges.data();
    }
    
    // ПОЛУЧЕНИЕ КОЛИЧЕСТВА РЁБЕР (в индексах)
    int getEdgeCount() const {
        return edges.size();
    }
    
    // РАЗМЕР ВЕРТИСНОГО БУФЕРА В БАЙТАХ
    int getVerticesSize() const {
        return vertices.size() * sizeof(float);
    }
    
    // РАЗМЕР БУФЕРА РЁБЕР В БАЙТАХ
    int getEdgesSize() const {
        return edges.size() * sizeof(unsigned int);
    }
};

unsigned int compileShader(unsigned int type, const char* source) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cout << "Ошибка компиляции шейдера: " << infoLog << std::endl;
    }
    
    return shader;
}

unsigned int createShaderProgram() {
    unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    int success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "Ошибка линковки шейдерной программы: " << infoLog << std::endl;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return shaderProgram;
}

int main() {
    // ИНИЦИАЛИЗАЦИЯ GLFW
    if(!glfwInit()) {
        std::cout << "Не удалось инициализировать GLFW" << std::endl;
        return -1;
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    GLFWwindow* window = glfwCreateWindow(800, 600, "WIREFRAME SQUARE", NULL, NULL);
    if (!window) {
        std::cout << "Не удалось создать окно GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    
    glewExperimental = GL_TRUE;
    if(glewInit() != GLEW_OK) {
        std::cout << "Не удалось инициализировать GLEW" << std::endl;
        return -1;
    }
    
    // СОЗДАНИЕ ФИГУРЫ И НАСТРОЙКА КВАДРАТА
    ShapeBuilder square;
    square.createSquare(0.0f, 0.0f, 0.8f); // квадрат в центре размером 0.8
    
    std::cout << "Вершин создано: " << square.getVertexCount() << std::endl;
    std::cout << "Рёбер создано: " << square.getEdgeCount() / 2 << std::endl;
    
    // СОЗДАНИЕ И НАСТРОЙКА VAO/VBO/EBO
    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    glBindVertexArray(VAO);
    
    // ВЕРТИСНЫЙ БУФЕР
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, square.getVerticesSize(), square.getVertices(), GL_STATIC_DRAW);
    
    // БУФЕР ИНДЕКСОВ (РЁБРА)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, square.getEdgesSize(), square.getEdges(), GL_STATIC_DRAW);
    
    // НАСТРОЙКА АТРИБУТОВ ВЕРШИН
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // СОЗДАНИЕ ШЕЙДЕРНОЙ ПРОГРАММЫ
    unsigned int shaderProgram = createShaderProgram();
    
    // НАСТРОЙКА OPENGL ДЛЯ ОТОБРАЖЕНИЯ ЛИНИЙ
    glLineWidth(2.0f); // Толщина линий
    
    std::cout << "Белый квадрат (только рёбра) должен отобразиться в центре экрана!" << std::endl;
    
    // ЦИКЛ РЕНДЕРИНГА
    while(!glfwWindowShouldClose(window)) {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        
        // ОТРИСОВКА ЛИНИЙ (РЁБЕР) вместо заполненных треугольников
        glDrawElements(GL_LINES, square.getEdgeCount(), GL_UNSIGNED_INT, 0);
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    // ОЧИСТКА РЕСУРСОВ
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);
    
    glfwTerminate();
    return 0;
}