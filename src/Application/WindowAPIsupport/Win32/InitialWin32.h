#ifndef INITIALWIN32_H
#define INITIALWIN32_H

#include <windows.h>
#include <GL/gl.h>

class InitialWin32 {
private:
    HWND hwnd;
    HDC hdc;
    HGLRC hrc;
    HMENU hMenu;

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
    InitialWin32() : hwnd(nullptr), hdc(nullptr), hrc(nullptr), hMenu(nullptr) {}
    
    HWND getHWND() { return hwnd; }
    
    static InitialWin32* createWindow(int width, int height, const char* title);
    
    void pollEvents() {
        MSG msg;
        while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    
    HDC getHDC() const { return hdc; }
    
    void swapBuffers() {
        SwapBuffers(hdc);
    }
    
    bool shouldClose() {
        MSG msg;
        return PeekMessage(&msg, 0, WM_QUIT, WM_QUIT, PM_NOREMOVE);
    }
    
    HMENU getMenu() const { return hMenu; }
    
    ~InitialWin32() {
        if (hMenu) DestroyMenu(hMenu);
        wglMakeCurrent(NULL, NULL);
        if(hrc) wglDeleteContext(hrc);
        if(hdc) ReleaseDC(hwnd, hdc);
        if(hwnd) DestroyWindow(hwnd);
    }
};

#endif