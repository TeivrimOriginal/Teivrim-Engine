#ifndef INITIALGLFW_H
#define INITIALGLFW_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>

class InitialGLFW {
public:
    static GLFWwindow* createWindow(int width, int height, const char* title);
};

#endif