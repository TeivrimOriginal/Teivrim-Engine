#ifndef APPLICATION_H
#define APPLICATION_H


#include "../Control/camera.h"
#include "../Application/WindowAPIsupport/Win32/InitialWin32.h"
#include <glm/glm.hpp>
#include <string>
using namespace std;
class Application {
public:
    Application();
    bool createApplication();

    InitialWin32* getWindow32() const { return win32Window; }
    Camera& getCamera() { return camera; }
    void processInput(float deltaTime);
private:
    Camera camera;
    InitialWin32* win32Window;

    float lastX;
    float lastY;
    bool firstMouse;
    float deltaTime;
    float lastFrame;
};

#endif