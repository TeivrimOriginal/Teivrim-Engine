#include "ProjectManager.h"
#include "../Asset/AssetManager.h"
#include <shlobj.h>
#include <fstream>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

void ProjectManager::ShowCreateProjectDialog(HWND parent, std::function<void(const std::string&)> onCreated) {
    DialogData data;
    data.callback = onCreated;
    
    HWND hwnd = CreateWindowExA(WS_EX_DLGMODALFRAME, "STATIC", "Create Project",
                                 WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
                                 CW_USEDEFAULT, CW_USEDEFAULT, 450, 380,
                                 parent, NULL, GetModuleHandle(NULL), &data);
    
    if (!hwnd) return;
    
    RECT rect; GetWindowRect(hwnd, &rect);
    int w = rect.right - rect.left, h = rect.bottom - rect.top;
    SetWindowPos(hwnd, NULL, (GetSystemMetrics(SM_CXSCREEN)-w)/2, (GetSystemMetrics(SM_CYSCREEN)-h)/2, 0, 0, SWP_NOSIZE);
    SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)DialogProc);
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&data);
    
    int y = 15;
    CreateWindowA("STATIC", "Project Name:", WS_CHILD | WS_VISIBLE, 15, y, 100, 20, hwnd, NULL, NULL, NULL);
    data.nameEdit = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER, 130, y, 290, 22, hwnd, NULL, NULL, NULL);
    y += 40;
    CreateWindowA("STATIC", "Project Type:", WS_CHILD | WS_VISIBLE, 15, y, 100, 20, hwnd, NULL, NULL, NULL);
    data.typeEdit = CreateWindowA("COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 130, y, 290, 100, hwnd, NULL, NULL, NULL);
    SendMessageA(data.typeEdit, CB_ADDSTRING, 0, (LPARAM)"3D Game");
    SendMessageA(data.typeEdit, CB_ADDSTRING, 0, (LPARAM)"2D Game");
    SendMessageA(data.typeEdit, CB_ADDSTRING, 0, (LPARAM)"Application");
    SendMessageA(data.typeEdit, CB_SETCURSEL, 0, 0);
    y += 40;
    CreateWindowA("STATIC", "Engine Version:", WS_CHILD | WS_VISIBLE, 15, y, 100, 20, hwnd, NULL, NULL, NULL);
    data.versionEdit = CreateWindowA("EDIT", "1.0.0", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_READONLY, 130, y, 290, 22, hwnd, NULL, NULL, NULL);
    y += 40;
    CreateWindowA("STATIC", "Description:", WS_CHILD | WS_VISIBLE, 15, y, 100, 20, hwnd, NULL, NULL, NULL);
    data.descEdit = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE, 130, y, 290, 50, hwnd, NULL, NULL, NULL);
    y += 70;
    CreateWindowA("STATIC", "Directory:", WS_CHILD | WS_VISIBLE, 15, y, 100, 20, hwnd, NULL, NULL, NULL);
    data.dirEdit = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER, 130, y, 240, 22, hwnd, NULL, NULL, NULL);
    CreateWindowA("BUTTON", "...", WS_CHILD | WS_VISIBLE, 375, y, 50, 22, hwnd, (HMENU)100, NULL, NULL);
    y += 50;
    CreateWindowA("BUTTON", "Create", WS_CHILD | WS_VISIBLE, 250, y, 80, 28, hwnd, (HMENU)1, NULL, NULL);
    CreateWindowA("BUTTON", "Cancel", WS_CHILD | WS_VISIBLE, 340, y, 80, 28, hwnd, (HMENU)2, NULL, NULL);
    
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (!IsWindow(hwnd)) break;
    }
}

LRESULT CALLBACK ProjectManager::DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static DialogData* data = nullptr;
    if (msg == WM_INITDIALOG) { data = (DialogData*)lParam; SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)data); return TRUE; }
    data = (DialogData*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    
    switch (msg) {
        case WM_COMMAND:
            if (LOWORD(wParam) == 1 && HIWORD(wParam) == BN_CLICKED) {
                char name[256] = {0}, type[256] = {0}, version[256] = {0}, desc[512] = {0}, dir[MAX_PATH] = {0};
                GetWindowTextA(data->nameEdit, name, 256);
                GetWindowTextA(data->typeEdit, type, 256);
                GetWindowTextA(data->versionEdit, version, 256);
                GetWindowTextA(data->descEdit, desc, 512);
                GetWindowTextA(data->dirEdit, dir, MAX_PATH);
                if (strlen(name) > 0 && strlen(dir) > 0) {
                    Instance().CreateProject(name, type, version, desc, dir);
                    if (data->callback) data->callback(name);
                }
                DestroyWindow(hwnd);
            } else if (LOWORD(wParam) == 2) {
                DestroyWindow(hwnd);
            } else if (LOWORD(wParam) == 100) {
                BROWSEINFOA bi = {0};
                bi.lpszTitle = "Select Project Directory";
                bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
                LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
                if (pidl) {
                    char path[MAX_PATH];
                    SHGetPathFromIDListA(pidl, path);
                    SetWindowTextA(data->dirEdit, path);
                    CoTaskMemFree(pidl);
                }
            }
            break;
        case WM_CLOSE: DestroyWindow(hwnd); break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void ProjectManager::CreateProject(const std::string& name, const std::string& type, 
                                   const std::string& version, const std::string& desc,
                                   const std::string& directory) {
    std::string projectPath = directory + "\\" + name;
    CreateDirectoryA(projectPath.c_str(), NULL);
    CreateDirectoryA((projectPath + "\\Assets").c_str(), NULL);
    CreateDirectoryA((projectPath + "\\System\\Data\\Interface\\Grid").c_str(), NULL);
    
    time_t now = time(nullptr); struct tm timeinfo; localtime_s(&timeinfo, &now);
    std::ostringstream timeStr;
    timeStr << std::put_time(&timeinfo, "%Y-%m-%d %H:%M:%S");
    
    std::ofstream(projectPath + "\\project.json") << "{\"name\":\"" << name << "\",\"type\":\"" << type << "\"}";
    std::ofstream(projectPath + "\\meta.json") << "{\"created\":\"" << timeStr.str() << "\"}";
    
    AssetManager::Instance().LoadProject(projectPath);
    MessageBoxA(NULL, ("Project created at:\n" + projectPath).c_str(), "Success", MB_OK);
}