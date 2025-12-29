
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>

#include <glew.h>
#include <glfw3.h>

// Максимальное количество вершин и индексов, которые можно передать через uniform.
// В реальных задачах это очень маленькое ограничение.
#define MAX_VERTICES_UNIFORM 100
#define MAX_INDICES_UNIFORM 200

// Шейдеры
const char* vertexShaderSource = R"(
#version 330 core
// u_vertices - массив всех координат вершин
uniform vec2 u_vertices[100];
// u_indices - массив индексов, которые указывают, какие вершины соединять
uniform int u_indices[200];
// u_transform - матрица преобразования (например, идентичность для 2D)
uniform mat4 u_transform;
// u_color - цвет для отрисовки
uniform vec4 u_color;                   

out vec4 fragmentColor;

void main() {
    // gl_VertexID - это встроенная переменная, которая содержит индекс текущей вершины,
    // которую OpenGL обрабатывает при вызове glDrawArrays.
    // Мы используем его для доступа к массиву индексов (u_indices),
    // который затем дает нам фактический индекс вершины в массиве u_vertices.
    gl_Position = u_transform * vec4(u_vertices[u_indices[gl_VertexID]], 0.0, 1.0);
    fragmentColor = u_color;
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
in vec4 fragmentColor;
out vec4 color;

void main() {
    color = fragmentColor;
}
)";

// --- Вспомогательные функции для компиляции шейдеров ---
unsigned int compileShader(GLenum type, const char* source) {
    unsigned int id = glCreateShader(type);
    glShaderSource(id, 1, &source, nullptr);
    glCompileShader(id);

    int result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE) {
        int length;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
        char* message = (char*)alloca(length * sizeof(char));
        glGetShaderInfoLog(id, length, &length, message);
        std::cerr << "Failed to compile " << (type == GL_VERTEX_SHADER ? "vertex" : "fragment") << " shader!" << std::endl;
        std::cerr << message << std::endl;
        glDeleteShader(id);
        return 0;
    }
    return id;
}

unsigned int createShader(const char* vertexShader, const char* fragmentShader) {
    unsigned int program = glCreateProgram();
    unsigned int vs = compileShader(GL_VERTEX_SHADER, vertexShader);
    unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fragmentShader);

    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glValidateProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);

    return program;
}
// --------------------------------------------------------

int main() {
    // --- 1. Инициализация GLFW ---
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Simple Uniform Renderer", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // --- 2. Инициализация GLEW ---
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;

    // --- 3. Чтение данных из data.txt ---
    std::ifstream file("data.txt");
    if (!file.is_open()) {
        std::cerr << "Error: Could not open data.txt" << std::endl;
        glfwTerminate();
        return -1;
    }

    std::vector<float> vertices_data; // Будет хранить X, Y координаты
    std::vector<int> indices_data;   // Будет хранить индексы вершин для линий

    std::string line;
    // Чтение вершин (первая строка)
    if (std::getline(file, line)) {
        std::stringstream ss(line);
        float val;
        while (ss >> val) {
            vertices_data.push_back(val);
        }
    } else {
        std::cerr << "Error: Could not read vertices line from data.txt" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Чтение индексов (вторая строка)
    if (std::getline(file, line)) {
        std::stringstream ss(line);
        int val;
        while (ss >> val) {
            indices_data.push_back(val);
        }
    } else {
        std::cerr << "Error: Could not read indices line from data.txt" << std::endl;
        glfwTerminate();
        return -1;
    }
    file.close();

    // Проверка, что данные не превышают лимиты uniform массивов
    if (vertices_data.size() / 2 > MAX_VERTICES_UNIFORM || indices_data.size() > MAX_INDICES_UNIFORM) {
        std::cerr << "Error: Data exceeds uniform array limits. Adjust MAX_VERTICES_UNIFORM/MAX_INDICES_UNIFORM." << std::endl;
        glfwTerminate();
        return -1;
    }
    
    // --- 4. Создание и активация шейдерной программы ---
    unsigned int shaderProgram = createShader(vertexShaderSource, fragmentShaderSource);
    glUseProgram(shaderProgram);

    // --- 5. Получение местоположения uniform-переменных ---
    GLint verticesLoc = glGetUniformLocation(shaderProgram, "u_vertices");
    GLint indicesLoc = glGetUniformLocation(shaderProgram, "u_indices");
    GLint transformLoc = glGetUniformLocation(shaderProgram, "u_transform");
    GLint colorLoc = glGetUniformLocation(shaderProgram, "u_color");

    // Простейшая идентичная матрица преобразования (для 2D ничего не меняем)
    // float transform[16] = {
    //     1.0f, 0.0f, 0.0f, 0.0f,
    //     0.0f, 1.0f, 0.0f, 0.0f,
    //     0.0f, 0.0f, 1.0f, 0.0f,
    //     0.0f, 0.0f, 0.0f, 1.0f
    // };
    // Если хочешь подвинуть/масштабировать, измени ее. Например, для масштаба 0.5:
    float transform[16] = {
        0.5f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.5f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };


    // --- 6. Создание VAO (даже если он пустой, он нужен) ---
    // Для glDrawArrays нужен активный VAO, даже если к нему ничего не привязано,
    // когда данные идут через uniforms.
    unsigned int vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // --- 7. Основной цикл рендеринга ---
    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT); // Очистка экрана
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Черный фон

        // Передача данных в uniform-переменные
        // glUniform2fv для vec2 массивов (vertices_data содержит floats, по 2 на вершину)
        glUniform2fv(verticesLoc, vertices_data.size() / 2, vertices_data.data());
        // glUniform1iv для int массивов (indices_data)
        glUniform1iv(indicesLoc, indices_data.size(), indices_data.data());
        // glUniformMatrix4fv для матрицы преобразования
        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, transform);
        // glUniform4f для цвета (красный)
        glUniform4f(colorLoc, 1.0f, 0.0f, 0.0f, 1.0f); // Красный цвет

        // Отрисовка линий
        // glDrawArrays использует gl_VertexID, который будет от 0 до indices_data.size()-1.
        // Каждый gl_VertexID будет соответствовать элементу в u_indices,
        // который, в свою очередь, указывает на u_vertices.
        glDrawArrays(GL_LINES, 0, indices_data.size()); 

        glfwSwapBuffers(window); // Обновление буферов (показ нового кадра)
        glfwPollEvents();        // Обработка событий (ввод, закрытие окна)
    }

    // --- 8. Очистка ресурсов ---
    glDeleteProgram(shaderProgram);
    glDeleteVertexArrays(1, &vao); // Удаляем VAO
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
