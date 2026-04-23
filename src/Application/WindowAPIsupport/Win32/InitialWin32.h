#ifndef INITIALWIN32_H
#define INITIALWIN32_H

#include <windows.h>
#include <vector>
#include <string>
#include <functional>
#include <iostream>

#ifdef VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#endif

class InitialWin32 {
private:
    HWND hwnd;
    HDC hdc;
    HGLRC hrc;
    HMENU hMenu;
    HINSTANCE hInstance;
    int windowWidth;
    int windowHeight;

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        InitialWin32* pThis = (InitialWin32*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        
        if (pThis) {
            switch(msg) {
                case WM_COMMAND:
                    std::cout << "[WindowProc] WM_COMMAND received, ID: " << LOWORD(wParam) << std::endl;
                    if (LOWORD(wParam) == 1) {
                        std::cout << "[WindowProc] New Project clicked" << std::endl;
                        if (pThis->onNewProject) pThis->onNewProject();
                    }
                    else if (LOWORD(wParam) == 2) {
                        std::cout << "[WindowProc] Open Project clicked" << std::endl;
                        if (pThis->onOpenProject) pThis->onOpenProject();
                    }
                    else if (LOWORD(wParam) == 3) {
                        std::cout << "[WindowProc] Exit clicked" << std::endl;
                        DestroyWindow(hwnd);
                    }
                    break;
                    
                case WM_SIZE: {
                    int width = LOWORD(lParam);
                    int height = HIWORD(lParam);
                    if (width > 0 && height > 0) {
                        pThis->windowWidth = width;
                        pThis->windowHeight = height;
                        if (pThis->onResize) {
                            pThis->onResize(width, height);
                        }
                    }
                    break;
                }
                    
                case WM_CLOSE:
                    std::cout << "[WindowProc] WM_CLOSE" << std::endl;
                    DestroyWindow(hwnd);
                    break;
                    
                case WM_DESTROY:
                    std::cout << "[WindowProc] WM_DESTROY" << std::endl;
                    PostQuitMessage(0);
                    break;
            }
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

public:
    InitialWin32() : hwnd(nullptr), hdc(nullptr), hrc(nullptr), 
                     hMenu(nullptr), hInstance(nullptr), 
                     windowWidth(800), windowHeight(600),
                     onNewProject(nullptr), onOpenProject(nullptr), onResize(nullptr) {}
    
    HWND getHWND() { return hwnd; }
    HDC getHDC() const { return hdc; }
    HINSTANCE getHInstance() const { return hInstance; }
    HMENU getMenu() const { return hMenu; }
    int getWidth() const { return windowWidth; }
    int getHeight() const { return windowHeight; }
    void setWindowSize(int width, int height) { windowWidth = width; windowHeight = height; }
    int getClientWidth() const { return windowWidth; }
    int getClientHeight() const { return windowHeight; }
    
    std::function<void()> onNewProject;
    std::function<void()> onOpenProject;
    std::function<void(int, int)> onResize;
    
    static InitialWin32* createWindow(int width, int height, const char* title);
    
    void pollEvents() {
        MSG msg;
        while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    
    void swapBuffers() {
        if (hrc) SwapBuffers(hdc);
    }
    
    bool shouldClose() {
        MSG msg;
        return PeekMessage(&msg, 0, WM_QUIT, WM_QUIT, PM_NOREMOVE);
    }
    
    void getFramebufferSize(int* width, int* height) {
        *width = windowWidth;
        *height = windowHeight;
    }
    
    std::vector<const char*> getRequiredExtensions() {
        std::vector<const char*> extensions;
#ifdef VK_USE_PLATFORM_WIN32_KHR
        extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
        extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif
        return extensions;
    }
    
    bool createVulkanSurface(VkInstance instance, VkSurfaceKHR* surface);
    
    ~InitialWin32() {
        if (hMenu) DestroyMenu(hMenu);
        if (hrc) {
            wglMakeCurrent(NULL, NULL);
            wglDeleteContext(hrc);
        }
        if (hdc) ReleaseDC(hwnd, hdc);
        if (hwnd) DestroyWindow(hwnd);
    }
};

#endif