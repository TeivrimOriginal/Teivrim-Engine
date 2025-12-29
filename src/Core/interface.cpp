// interface.cpp
#include "interface.h"
#include <iostream>

Interface::Interface() 
    : shaderProgram(0), borderVAO(0), borderVBO(0), 
      screenWidth(800), screenHeight(600), window(nullptr) {}

Interface::~Interface() {
    cleanup();
}

void Interface::initialize(GLFWwindow* window) {
    this->window = window;
    
    // Компилируем шейдеры
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    
    // Создаем программу
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    // Получаем размеры окна
    glfwGetWindowSize(window, &screenWidth, &screenHeight);
    
    // Создаем VAO/VBO для обводки
    createBorderVAO();
    
    std::cout << "Interface initialized" << std::endl;
}

void Interface::createBorderVAO() {
    // Координаты для обводки по краям экрана (в NDC: -1..1)
    // Мы создаем 4 линии по краям экрана
    float borderThickness = 0.01f; // Толщина обводки (в NDC)
    
    // Верхняя линия
    float topLine[] = {
        -1.0f, 1.0f - borderThickness,
         1.0f, 1.0f - borderThickness,
         1.0f, 1.0f,
        -1.0f, 1.0f,
    };
    
    // Нижняя линия
    float bottomLine[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
         1.0f, -1.0f + borderThickness,
        -1.0f, -1.0f + borderThickness,
    };
    
    // Левая линия
    float leftLine[] = {
        -1.0f, -1.0f,
        -1.0f + borderThickness, -1.0f,
        -1.0f + borderThickness, 1.0f,
        -1.0f, 1.0f,
    };
    
    // Правая линия
    float rightLine[] = {
        1.0f - borderThickness, -1.0f,
        1.0f, -1.0f,
        1.0f, 1.0f,
        1.0f - borderThickness, 1.0f,
    };
    
    // Объединяем все линии в один буфер
    float borderVertices[32]; // 4 линии * 4 вершины * 2 координаты
    
    // Копируем вершины
    for (int i = 0; i < 8; i++) {
        borderVertices[i] = topLine[i];
        borderVertices[i + 8] = bottomLine[i];
        borderVertices[i + 16] = leftLine[i];
        borderVertices[i + 24] = rightLine[i];
    }
    
    glGenVertexArrays(1, &borderVAO);
    glGenBuffers(1, &borderVBO);
    
    glBindVertexArray(borderVAO);
    glBindBuffer(GL_ARRAY_BUFFER, borderVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(borderVertices), borderVertices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);
}

void Interface::render() {
    if (!shaderProgram) return;
    
    // Сохраняем текущие настройки OpenGL
    glDisable(GL_DEPTH_TEST);
    
    // Используем шейдер интерфейса
    glUseProgram(shaderProgram);
    
    // Устанавливаем цвет обводки (синий)
    GLint colorLoc = glGetUniformLocation(shaderProgram, "borderColor");
    glUniform3f(colorLoc, 0.0f, 0.5f, 1.0f); // Синий цвет
    
    // Рисуем обводку
    glBindVertexArray(borderVAO);
    
    // Рисуем каждую линию отдельно
    for (int i = 0; i < 4; i++) {
        glDrawArrays(GL_TRIANGLE_FAN, i * 4, 4);
    }
    
    glBindVertexArray(0);
    
    // Восстанавливаем настройки
    glEnable(GL_DEPTH_TEST);
}

void Interface::cleanup() {
    if (borderVAO) {
        glDeleteVertexArrays(1, &borderVAO);
        borderVAO = 0;
    }
    
    if (borderVBO) {
        glDeleteBuffers(1, &borderVBO);
        borderVBO = 0;
    }
    
    if (shaderProgram) {
        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
    }
}

void Interface::setWindowSize(int width, int height) {
    screenWidth = width;
    screenHeight = height;
}