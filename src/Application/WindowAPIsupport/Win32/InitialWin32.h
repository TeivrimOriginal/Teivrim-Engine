#ifndef INITIALWIN32_H
#define INITIALWIN32_H

#include <windows.h>
#include <GL/gl.h>

class InitialWin32 {
private:
    HWND hwnd;
    HDC hdc;
    HGLRC hrc;

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch(msg) {
            case WM_CLOSE:
                DestroyWindow(hwnd);
                break;
            case WM_DESTROY:
                PostQuitMessage(0);
                break;
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

public:
    HWND getHWND() { return hwnd; }
    static InitialWin32* createWindow(int width, int height, const char* title);
    
    void pollEvents() {
        MSG msg;
        while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    HDC getHDC() const { return hdc; }  // Добавить этот метод
    void swapBuffers() {
        SwapBuffers(hdc);
    }
    
    bool shouldClose() {
        MSG msg;
        return PeekMessage(&msg, 0, WM_QUIT, WM_QUIT, PM_NOREMOVE);
    }
    
    ~InitialWin32() {
        wglMakeCurrent(NULL, NULL);
        if(hrc) wglDeleteContext(hrc);
        if(hdc) ReleaseDC(hwnd, hdc);
        if(hwnd) DestroyWindow(hwnd);
    }
};

#endif