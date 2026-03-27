#include "InitialWin32.h"

InitialWin32* InitialWin32::createWindow(int width, int height, const char* title) {
    InitialWin32* win = new InitialWin32();
    
    HINSTANCE hInst = GetModuleHandle(NULL);
    
    win->hMenu = CreateMenu();
    HMENU hFileMenu = CreatePopupMenu();
    HMENU hEditMenu = CreatePopupMenu();
    HMENU hWindowMenu = CreatePopupMenu();
    
    AppendMenuA(hFileMenu, MF_STRING, 1, "New");
    AppendMenuA(hFileMenu, MF_STRING, 2, "Open");
    AppendMenuA(hFileMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuA(hFileMenu, MF_STRING, 3, "Exit");
    
    AppendMenuA(hEditMenu, MF_STRING, 4, "Undo");
    AppendMenuA(hEditMenu, MF_STRING, 5, "Redo");
    
    AppendMenuA(hWindowMenu, MF_STRING, 6, "Add New Panel");
    AppendMenuA(hWindowMenu, MF_STRING, 7, "Floating Panel");
    
    AppendMenuA(win->hMenu, MF_POPUP, (UINT_PTR)hFileMenu, "File");
    AppendMenuA(win->hMenu, MF_POPUP, (UINT_PTR)hEditMenu, "Edit");
    AppendMenuA(win->hMenu, MF_POPUP, (UINT_PTR)hWindowMenu, "Window");
    
    WNDCLASSA wc = {};
    wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = InitialWin32::WindowProc;
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
        NULL, win->hMenu, hInst, win
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
        24,
        8,
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