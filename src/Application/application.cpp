#include "application.h"
#include "../Control/Input.h"
#include "WindowAPIsupport/GLFW/InitialGLFW.h"
#include "WindowAPIsupport/Win32/InitialWin32.h"
#include <iostream>
#include <chrono>
#include <string>
#include <conio.h>
using namespace std;
Application::Application() 
    : window(0),
      camera(glm::vec3(0.0f, 0.0f, 5.0f)),
      lastX(400.0f), lastY(300.0f), firstMouse(true),
      deltaTime(0.0f), lastFrame(0.0f) {


      }

bool Application::createApplication() {
    cout << "Create application" << endl;
    cout << "Выберите оконное API(Win32 / GLFW)" << endl;
    cin >> modeWinAPI;
    cout << endl; 
    
    if(modeWinAPI == "glfw") {
        window = InitialGLFW::createWindow(1920, 1080, "3D Model Viewer");
        if (!window) return false;
        
        glfwSetWindowUserPointer(window, this);
        glfwSetCursorPosCallback(window, Input::processMouse);
        glfwSetScrollCallback(window, Input::processScroll);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        win32Window = nullptr; 


        return true;
    }

        if(modeWinAPI == "Win32") { 
            win32Window = InitialWin32::createWindow(1920, 1080, "3D Model Viewer"); 
            window = nullptr; 
            return win32Window != nullptr;
        } else {
        cout << " ne pravilno";
        int hui;
        cin >> hui;
    }
    
    return false;
}

void Application::processInput(float deltaTime) {
    // заготов 0чка
}