#ifndef INITIALWIN32_H
#define INITIALWIN32_H

#include <windows.h>
#include <vector>
#include <string>

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
        InitialWin32* pThis = nullptr;
        
        if (msg == WM_NCCREATE) {
            CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
            pThis = reinterpret_cast<InitialWin32*>(pCreate->lpCreateParams);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        } else {
            pThis = reinterpret_cast<InitialWin32*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        }
        
        if (pThis) {
            switch(msg) {
                case WM_SIZE:
                    pThis->windowWidth = LOWORD(lParam);
                    pThis->windowHeight = HIWORD(lParam);
                    break;
                case WM_COMMAND:
                    if (LOWORD(wParam) == 1) {
                        MessageBox(hwnd, "New Panel Added", "Window", MB_OK);
                    }
                    break;
                case WM_CLOSE:
                    DestroyWindow(hwnd);
                    break;
                case WM_DESTROY:
                    PostQuitMessage(0);
                    break;
            }
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

public:
    InitialWin32() : hwnd(nullptr), hdc(nullptr), hrc(nullptr), 
                     hMenu(nullptr), hInstance(nullptr), 
                     windowWidth(800), windowHeight(600) {}
    
    HWND getHWND() { return hwnd; }
    HDC getHDC() const { return hdc; }
    HINSTANCE getHInstance() const { return hInstance; }
    HMENU getMenu() const { return hMenu; }
    int getWidth() const { return windowWidth; }
    int getHeight() const { return windowHeight; }
    
    static InitialWin32* createWindow(int width, int height, const char* title);
    
    void pollEvents() {
        MSG msg;
        while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    
    void swapBuffers() {
        if (hrc) {
            SwapBuffers(hdc);
        }
    }
    
    bool shouldClose() {
        MSG msg;
        return PeekMessage(&msg, 0, WM_QUIT, WM_QUIT, PM_NOREMOVE);
    }
    
    // ТОЛЬКО ОДИН метод getFramebufferSize (удали дубликат)
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