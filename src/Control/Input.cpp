#include "Input.h"
#include <windows.h>

Input::Input(Application& app) : app(app), window(app.getWindow()), camera(app.getCamera()) {
    app1 = (Application*)glfwGetWindowUserPointer(window);
}

void Input::processScroll(GLFWwindow* window, double xoffset, double yoffset) {
    Application* app = (Application*)glfwGetWindowUserPointer(window);
    if (!app) return;
    app->getCamera().ProcessMouseScroll((float)yoffset);
}

void Input::processMouse(GLFWwindow* window, double xpos, double ypos) {
    Application* app = (Application*)glfwGetWindowUserPointer(window);
    if (!app) return;
    
    static bool firstMouse = true;
    static float lastX = 400.0f;
    static float lastY = 300.0f;
    
    if (firstMouse) {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }
    
    float xoffset = (float)xpos - lastX;
    float yoffset = lastY - (float)ypos;
    
    lastX = (float)xpos;
    lastY = (float)ypos;
    
    app->getCamera().ProcessMouseMovement(xoffset, yoffset);
}

void Input::processInput(float deltaTime) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
    
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camera.ProcessKeyboard(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        camera.ProcessKeyboard(DOWN, deltaTime);
}

// WIN32 ВЕРСИЯ
void Input::processInputWin32(float deltaTime, HWND hwnd) {
    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
        PostMessage(hwnd, WM_CLOSE, 0, 0);
    }
    
    if (GetAsyncKeyState('W') & 0x8000)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (GetAsyncKeyState('S') & 0x8000)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (GetAsyncKeyState('A') & 0x8000)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (GetAsyncKeyState('D') & 0x8000)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if (GetAsyncKeyState(VK_SPACE) & 0x8000)
        camera.ProcessKeyboard(UP, deltaTime);
    if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
        camera.ProcessKeyboard(DOWN, deltaTime);
}

void Input::processMouseWin32(float xpos, float ypos) {
    static bool firstMouse = true;
    static float lastX = 400.0f;
    static float lastY = 300.0f;
    
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