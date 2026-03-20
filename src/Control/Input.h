#ifndef INPUT_H
#define INPUT_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "camera.h"
#include "../Application/application.h"

class Input {
public:
    Input(Application& app);
    void processInput(float deltaTime);
    static void processMouse(GLFWwindow* window, double xpos, double ypos);
    static void processScroll(GLFWwindow* window, double xoffset, double yoffset);
    void processInputWin32(float deltaTime, HWND hwnd);
    void processMouseWin32(float xpos, float ypos);
private:
    Application* app1;
    Application& app;
    GLFWwindow* window;
    Camera& camera;
};

#endif