#include "application.h"
#include "../Control/Input.h"
#include "WindowAPIsupport/Win32/InitialWin32.h"
#include <iostream>
#include <chrono>
#include <string>
#include <conio.h>
using namespace std;
Application::Application() 
      : camera(glm::vec3(0.0f, 0.0f, 5.0f)),
      lastX(400.0f), lastY(300.0f), firstMouse(true),
      deltaTime(0.0f), lastFrame(0.0f) {

      }

bool Application::createApplication() {
    cout << "Create Win32 application" << endl;
    win32Window = InitialWin32::createWindow(1920, 1080, "3D Model Viewer");
    return win32Window != nullptr;
}

void Application::processInput(float deltaTime) {

}