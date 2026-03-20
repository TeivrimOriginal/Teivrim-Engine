#include "InitialWin32.h"

// УБИРАЕМ отдельную WindowProc - используем ту, что в классе

InitialWin32* InitialWin32::createWindow(int width, int height, const char* title) {
    InitialWin32* win = new InitialWin32();
    
    HINSTANCE hInst = GetModuleHandle(NULL);
    
    WNDCLASSA wc = {};
    wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = InitialWin32::WindowProc; // ЯВНО УКАЗЫВАЕМ WindowProc ИЗ КЛАССА
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "GLWin32Class";
    
    RegisterClassA(&wc);
    
    win->hwnd = CreateWindowExA(
        0,
        "GLWin32Class",
        title,
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        width, height,
        NULL, NULL, hInst, NULL
    );
    
    if (!win->hwnd) {
        delete win;
        return nullptr;
    }
    
    win->hdc = GetDC(win->hwnd);
    
    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA,
        32,
        0, 0, 0, 0, 0, 0,
        0, 0,
        0, 0, 0, 0, 0,
        24,                 // ГЛУБИНА 24 БИТА - ЭТО РЕШИТ ПРОБЛЕМУ
        8,                  // СТЕНСИЛ 8 БИТ
        0,
        PFD_MAIN_PLANE,
        0, 0, 0, 0
    };
    
    int pixelFormat = ChoosePixelFormat(win->hdc, &pfd);
    SetPixelFormat(win->hdc, pixelFormat, &pfd);
    
    win->hrc = wglCreateContext(win->hdc);
    wglMakeCurrent(win->hdc, win->hrc);
    
    ShowWindow(win->hwnd, SW_SHOW);
    UpdateWindow(win->hwnd);
    
    return win;
}