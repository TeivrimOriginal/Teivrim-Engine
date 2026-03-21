#ifndef INPUT_H
#define INPUT_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include "camera.h"
#include "../Application/application.h"

class Input {
public:
    Input(Application& app);
    void processInput(float deltaTime);
    void processInputWin32(float deltaTime, HWND hwnd);
    void processMouseWin32(float xpos, float ypos);
private:
    Application& app;
    Camera& camera;
};

#endif