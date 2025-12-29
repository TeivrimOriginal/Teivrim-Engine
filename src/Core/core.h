#ifndef CORE_H
#define CORE_H

#include "interfaces.h"
#include "renderer.h"
#include "parser.h"
#include <memory>
#include <vector>

// ============================================================================
// Базовый класс Core - абстрактный
// ============================================================================
class Core : public IInitializable, public IUpdatable {
public:
    Core() : running(false) {}
    virtual ~Core() = default;
    
    void run() {
        if (!initialize()) {
            std::cout << "Failed to initialize core!" << std::endl;
            return;
        }
        
        running = true;
        mainLoop();
        cleanup();
    }
    
    void stop() {
        running = false;
    }
    
    virtual bool initialize() override = 0;
    virtual void update(float deltaTime) override = 0;
    virtual void cleanup() override = 0;
    
protected:
    virtual void mainLoop() {
        float lastFrame = 0.0f;
        
        while (running) {
            float currentFrame = glfwGetTime();
            float deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;
            
            update(deltaTime);
            
            // Ограничение FPS (по желанию)
            // std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }
    
    bool running;
};

// ============================================================================
// Дочерний класс для управления приложением
// ============================================================================
class ApplicationCore : public Core {
public:
    ApplicationCore() : window(nullptr) {}
    
    bool initialize() override {
        // Инициализация GLFW
        if (!glfwInit()) {
            std::cout << "Failed to initialize GLFW!" << std::endl;
            return false;
        }
        
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        
        window = glfwCreateWindow(800, 600, "3D Model Viewer", nullptr, nullptr);
        if (!window) {
            std::cout << "Failed to create window!" << std::endl;
            glfwTerminate();
            return false;
        }
        
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1); // V-Sync
        
        // Инициализация GLEW
        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) {
            std::cout << "Failed to initialize GLEW!" << std::endl;
            return false;
        }
        
        std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
        
        return true;
    }
    
    void update(float deltaTime) override {
        // Основная логика приложения
        glfwPollEvents();
        running = !glfwWindowShouldClose(window);
    }
    
    void cleanup() override {
        if (window) {
            glfwDestroyWindow(window);
        }
        glfwTerminate();
    }
    
    GLFWwindow* getWindow() const { return window; }
    
private:
    GLFWwindow* window;
};

// ============================================================================
// Дочерний класс для рендеринга
// ============================================================================
class RenderingCore : public Core, public IRenderable {
public:
    RenderingCore(std::shared_ptr<ApplicationCore> appCore) 
        : appCore(appCore), shaderProgram(0) {}
    
    bool initialize() override {
        if (!appCore->getWindow()) {
            std::cout << "No window available for rendering!" << std::endl;
            return false;
        }
        
        // Инициализация рендерера
        renderer = std::make_unique<Renderer>();
        if (!renderer->initialize()) {
            std::cout << "Failed to initialize renderer!" << std::endl;
            return false;
        }
        
        // Компиляция шейдеров
        shaderProgram = initShaders();
        if (shaderProgram == 0) {
            std::cout << "Failed to compile shaders!" << std::endl;
            return false;
        }
        
        // Загрузка модели (можно вынести в конфигурацию)
        if (!modelParser.loadModel("resources/models/model.obj")) {
            std::cout << "Failed to load model! Using default..." << std::endl;
        }
        
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        
        return true;
    }
    
    void update(float deltaTime) override {
        // Обновление логики рендеринга
    }
    
    void cleanup() override {
        if (shaderProgram != 0) {
            glDeleteProgram(shaderProgram);
        }
        renderer->cleanup();
    }
    
    // IRenderable implementation
    void render(float deltaTime) override {
        beginFrame();
        
        // Рендеринг модели
        renderer->renderModel(modelParser, shaderProgram);
        
        // Рендеринг интерфейса поверх всего
        renderUI();
        
        endFrame();
    }
    
    void beginFrame() override {
        renderer->beginFrame();
    }
    
    void endFrame() override {
        renderer->endFrame();
    }
    
    void renderUI() {
        // Здесь будет отрисовка интерфейса
        // Можно использовать ImGui или простые OpenGL примитивы
    }
    
    Renderer* getRenderer() const { return renderer.get(); }
    ModelParser* getModelParser() { return &modelParser; }
    
private:
    std::shared_ptr<ApplicationCore> appCore;
    std::unique_ptr<Renderer> renderer;
    ModelParser modelParser;
    GLuint shaderProgram;
};

// ============================================================================
// Дочерний класс для обработки ввода
// ============================================================================
class InputCore : public Core, public IInputHandler {
public:
    InputCore(std::shared_ptr<ApplicationCore> appCore,
              std::shared_ptr<RenderingCore> renderingCore)
        : appCore(appCore), renderingCore(renderingCore) {}
    
    bool initialize() override {
        if (!appCore->getWindow()) {
            return false;
        }
        
        // Установка callback'ов
        GLFWwindow* window = appCore->getWindow();
        glfwSetWindowUserPointer(window, this);
        
        glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
            InputCore* inputCore = static_cast<InputCore*>(glfwGetWindowUserPointer(window));
            inputCore->keyCallback(window, key, scancode, action, mods);
        });
        
        glfwSetCursorPosCallback(window, [](GLFWwindow* window, double xpos, double ypos) {
            InputCore* inputCore = static_cast<InputCore*>(glfwGetWindowUserPointer(window));
            inputCore->mouseCallback(xpos, ypos);
        });
        
        glfwSetScrollCallback(window, [](GLFWwindow* window, double xoffset, double yoffset) {
            InputCore* inputCore = static_cast<InputCore*>(glfwGetWindowUserPointer(window));
            inputCore->scrollCallback(xoffset, yoffset);
        });
        
        // Скрываем курсор для FPS-камеры
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        
        return true;
    }
    
    void update(float deltaTime) override {
        processInput(deltaTime);
    }
    
    void cleanup() override {
        // Очистка ресурсов ввода
    }
    
    // IInputHandler implementation
    void processInput(float deltaTime) override {
        if (!renderingCore || !renderingCore->getRenderer()) return;
        
        renderingCore->getRenderer()->processInput(deltaTime);
    }
    
    void mouseCallback(double xpos, double ypos) override {
        if (!renderingCore || !renderingCore->getRenderer()) return;
        
        renderingCore->getRenderer()->mouseCallback(xpos, ypos);
    }
    
    void scrollCallback(double xoffset, double yoffset) override {
        if (!renderingCore || !renderingCore->getRenderer()) return;
        
        renderingCore->getRenderer()->scrollCallback(xoffset, yoffset);
    }
    
private:
    void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }
        
        // Дополнительная обработка клавиш может быть добавлена здесь
    }
    
    std::shared_ptr<ApplicationCore> appCore;
    std::shared_ptr<RenderingCore> renderingCore;
};

// ============================================================================
// Главный менеджер приложения
// ============================================================================
class AppManager {
public:
    AppManager() {
        // Создаем ядра в правильном порядке
        appCore = std::make_shared<ApplicationCore>();
        renderingCore = std::make_shared<RenderingCore>(appCore);
        inputCore = std::make_shared<InputCore>(appCore, renderingCore);
        
        // Собираем все ядра в вектор для удобного управления
        cores.push_back(appCore);
        cores.push_back(renderingCore);
        cores.push_back(inputCore);
    }
    
    bool initialize() {
        std::cout << "Initializing application..." << std::endl;
        
        // Инициализируем все ядра по порядку
        for (auto& core : cores) {
            if (!core->initialize()) {
                std::cout << "Failed to initialize core!" << std::endl;
                return false;
            }
        }
        
        std::cout << "Application initialized successfully!" << std::endl;
        return true;
    }
    
    void run() {
        std::cout << "Starting main loop..." << std::endl;
        
        float lastFrame = 0.0f;
        
        while (!glfwWindowShouldClose(appCore->getWindow())) {
            float currentFrame = glfwGetTime();
            float deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;
            
            // Обновление всех ядер
            for (auto& core : cores) {
                core->update(deltaTime);
            }
            
            // Рендеринг
            if (auto renderable = dynamic_cast<IRenderable*>(renderingCore.get())) {
                renderable->render(deltaTime);
            }
            
            // Ограничение FPS (опционально)
            // std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
        
        cleanup();
    }
    
    void cleanup() {
        std::cout << "Cleaning up..." << std::endl;
        
        // Очистка в обратном порядке
        for (auto it = cores.rbegin(); it != cores.rend(); ++it) {
            (*it)->cleanup();
        }
        
        std::cout << "Application terminated." << std::endl;
    }
    
    std::shared_ptr<ApplicationCore> getAppCore() const { return appCore; }
    std::shared_ptr<RenderingCore> getRenderingCore() const { return renderingCore; }
    std::shared_ptr<InputCore> getInputCore() const { return inputCore; }
    
private:
    std::shared_ptr<ApplicationCore> appCore;
    std::shared_ptr<RenderingCore> renderingCore;
    std::shared_ptr<InputCore> inputCore;
    std::vector<std::shared_ptr<Core>> cores;
};

#endif