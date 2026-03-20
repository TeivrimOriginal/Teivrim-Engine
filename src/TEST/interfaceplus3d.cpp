#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GL/glew.h>
#include <windows.h>

// Минимальный vertex shader
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 transform;
void main() {
    gl_Position = transform * vec4(aPos, 1.0);
}
)";

// Минимальный fragment shader
const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
uniform vec4 color;
void main() {
    FragColor = color;
}
)";

// Вершины для треугольника
float triangleVertices[] = {
    -0.5f, -0.5f, 0.0f,
     0.5f, -0.5f, 0.0f,
     0.0f,  0.5f, 0.0f
};

// Вершины для квадрата (UI элемент)
float uiVertices[] = {
    -1.0f, -1.0f, 0.0f,
     1.0f, -1.0f, 0.0f,
     1.0f,  1.0f, 0.0f,
    -1.0f, -1.0f, 0.0f,
     1.0f,  1.0f, 0.0f,
    -1.0f,  1.0f, 0.0f
};

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_DESTROY || msg == WM_CLOSE) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

GLuint CompileShader(const char* source, GLenum type) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    return shader;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Регистрация окна - используем LPCSTR
    WNDCLASSA wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "MinimalOpenGL";  // const char*, не LPCWSTR
    RegisterClassA(&wc);  // Используем RegisterClassA
    
    HWND hwnd = CreateWindowExA(0, "MinimalOpenGL", "OpenGL + UI",  // CreateWindowExA
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL, NULL, hInstance, NULL);
    
    // OpenGL setup
    HDC hdc = GetDC(hwnd);
    
    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR), 1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA, 32, 0,0,0,0,0,0,0,0,0,0,0,0,0,
        24, 8, 0, PFD_MAIN_PLANE, 0,0,0,0
    };
    
    int pf = ChoosePixelFormat(hdc, &pfd);
    SetPixelFormat(hdc, pf, &pfd);
    
    HGLRC hrc = wglCreateContext(hdc);
    wglMakeCurrent(hdc, hrc);
    
    glewInit();
    glViewport(0, 0, 800, 600);
    
    // Создание шейдерной программы
    GLuint vertexShader = CompileShader(vertexShaderSource, GL_VERTEX_SHADER);
    GLuint fragmentShader = CompileShader(fragmentShaderSource, GL_FRAGMENT_SHADER);
    
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    // Буферы для треугольника (3D сцена)
    GLuint VAO_tri, VBO_tri;
    glGenVertexArrays(1, &VAO_tri);
    glGenBuffers(1, &VBO_tri);
    
    glBindVertexArray(VAO_tri);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_tri);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangleVertices), triangleVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Буферы для UI
    GLuint VAO_ui, VBO_ui;
    glGenVertexArrays(1, &VAO_ui);
    glGenBuffers(1, &VBO_ui);
    
    glBindVertexArray(VAO_ui);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_ui);
    glBufferData(GL_ARRAY_BUFFER, sizeof(uiVertices), uiVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glUseProgram(shaderProgram);
    
    // Главный цикл
    MSG msg = {};
    float time = 0.0f;
    
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            time += 0.01f;
            
            // Очистка экрана
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            
            // Рендер UI панелей (по краям)
            glUseProgram(shaderProgram);
            glm::mat4 identity = glm::mat4(1.0f);
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "transform"), 1, GL_FALSE, glm::value_ptr(identity));
            
            // Левая панель (синяя)
            glViewport(0, 0, 100, 600);
            glUniform4f(glGetUniformLocation(shaderProgram, "color"), 0.2f, 0.3f, 0.8f, 1.0f);
            glBindVertexArray(VAO_ui);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            
            // Правая панель (зеленая)
            glViewport(700, 0, 100, 600);
            glUniform4f(glGetUniformLocation(shaderProgram, "color"), 0.2f, 0.8f, 0.3f, 1.0f);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            
            // Верхняя панель (красная)
            glViewport(100, 500, 600, 100);
            glUniform4f(glGetUniformLocation(shaderProgram, "color"), 0.8f, 0.3f, 0.2f, 1.0f);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            
            // Нижняя панель (желтая)
            glViewport(100, 0, 600, 100);
            glUniform4f(glGetUniformLocation(shaderProgram, "color"), 0.8f, 0.8f, 0.2f, 1.0f);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            
            // Рендер 3D сцены в центре (вращающийся треугольник)
            glViewport(100, 100, 600, 400);
            
            glm::mat4 transform = glm::rotate(glm::mat4(1.0f), time, glm::vec3(0.0f, 0.0f, 1.0f));
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "transform"), 1, GL_FALSE, glm::value_ptr(transform));
            glUniform4f(glGetUniformLocation(shaderProgram, "color"), 0.8f, 0.2f, 0.8f, 1.0f);
            
            glBindVertexArray(VAO_tri);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            
            SwapBuffers(hdc);
            Sleep(16); // ~60 FPS
        }
    }
    
    return 0;
}