#include "ProjectManager.h"
#include "../Asset/AssetManager.h"
#include <commdlg.h>
#include <shlobj.h>
#include <fstream>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

void ProjectManager::ShowCreateProjectDialog(HWND parent, std::function<void(const std::string&)> onCreated) {
    std::cout << "[ProjectManager] ShowCreateProjectDialog called" << std::endl;
    
    DialogData data;
    data.callback = onCreated;
    
    HWND hwnd = CreateWindowExA(WS_EX_DLGMODALFRAME, "STATIC", "Create Project",
                                 WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
                                 CW_USEDEFAULT, CW_USEDEFAULT, 450, 380,
                                 parent, NULL, GetModuleHandle(NULL), &data);
    
    if (!hwnd) {
        std::cout << "[ProjectManager] Failed to create dialog window!" << std::endl;
        return;
    }
    
    std::cout << "[ProjectManager] Dialog window created" << std::endl;
    
    RECT rect;
    GetWindowRect(hwnd, &rect);
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
    
    std::cout << "[ProjectManager] Dialog controls created, entering message loop" << std::endl;
    
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (!IsWindow(hwnd)) break;
    }
    
    std::cout << "[ProjectManager] Dialog closed" << std::endl;
}

LRESULT CALLBACK ProjectManager::DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    DialogData* data = (DialogData*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    
    switch (msg) {
        case WM_COMMAND: {
            std::cout << "[DialogProc] WM_COMMAND, ID: " << LOWORD(wParam) << std::endl;
            
            if (LOWORD(wParam) == 1 && HIWORD(wParam) == BN_CLICKED) {
                std::cout << "[DialogProc] Create button clicked" << std::endl;
                
                char name[256] = {0}, type[256] = {0}, version[256] = {0}, desc[512] = {0}, dir[MAX_PATH] = {0};
                GetWindowTextA(data->nameEdit, name, 256);
                GetWindowTextA(data->typeEdit, type, 256);
                GetWindowTextA(data->versionEdit, version, 256);
                GetWindowTextA(data->descEdit, desc, 512);
                GetWindowTextA(data->dirEdit, dir, MAX_PATH);
                
                std::cout << "[DialogProc] Name: " << name << std::endl;
                std::cout << "[DialogProc] Type: " << type << std::endl;
                std::cout << "[DialogProc] Version: " << version << std::endl;
                std::cout << "[DialogProc] Description: " << desc << std::endl;
                std::cout << "[DialogProc] Directory: " << dir << std::endl;
                
                if (strlen(name) > 0 && strlen(dir) > 0) {
                    Instance().CreateProject(name, type, version, desc, dir);
                    if (data->callback) {
                        std::cout << "[DialogProc] Calling callback with name: " << name << std::endl;
                        data->callback(name);
                    }
                } else {
                    std::cout << "[DialogProc] Name or directory is empty!" << std::endl;
                    MessageBoxA(hwnd, "Please enter project name and select directory", "Error", MB_OK);
                    break;
                }
                DestroyWindow(hwnd);
            }
            else if (LOWORD(wParam) == 2) {
                std::cout << "[DialogProc] Cancel button clicked" << std::endl;
                DestroyWindow(hwnd);
            }
            else if (LOWORD(wParam) == 100) {
                std::cout << "[DialogProc] Browse button clicked" << std::endl;
                
                BROWSEINFOA bi = {0};
                bi.lpszTitle = "Select Project Directory";
                bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
                LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
                if (pidl) {
                    char path[MAX_PATH];
                    SHGetPathFromIDListA(pidl, path);
                    SetWindowTextA(data->dirEdit, path);
                    CoTaskMemFree(pidl);
                    std::cout << "[DialogProc] Selected directory: " << path << std::endl;
                }
            }
            break;
        }
        case WM_CLOSE:
            std::cout << "[DialogProc] WM_CLOSE" << std::endl;
            DestroyWindow(hwnd);
            break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void ProjectManager::CreateProject(const std::string& name, const std::string& type, 
                                   const std::string& version, const std::string& desc,
                                   const std::string& directory) {
    std::cout << "[ProjectManager] CreateProject called" << std::endl;
    std::cout << "[ProjectManager] Name: " << name << std::endl;
    std::cout << "[ProjectManager] Type: " << type << std::endl;
    std::cout << "[ProjectManager] Version: " << version << std::endl;
    std::cout << "[ProjectManager] Description: " << desc << std::endl;
    std::cout << "[ProjectManager] Directory: " << directory << std::endl;
    
    std::string projectPath = directory + "\\" + name;
    std::cout << "[ProjectManager] Creating project at: " << projectPath << std::endl;
    
    BOOL result = CreateDirectoryA(projectPath.c_str(), NULL);
    if (!result) {
        std::cout << "[ProjectManager] Failed to create directory! Error: " << GetLastError() << std::endl;
        MessageBoxA(NULL, ("Failed to create directory: " + projectPath).c_str(), "Error", MB_OK);
        return;
    }
    std::cout << "[ProjectManager] Directory created" << std::endl;
    
    // Создаём структуру папок проекта
    CreateDirectoryA((projectPath + "\\Assets").c_str(), NULL);
    CreateDirectoryA((projectPath + "\\Assets\\Models").c_str(), NULL);
    CreateDirectoryA((projectPath + "\\Assets\\Textures").c_str(), NULL);
    CreateDirectoryA((projectPath + "\\Assets\\Scripts").c_str(), NULL);
    CreateDirectoryA((projectPath + "\\Assets\\Scenes").c_str(), NULL);
    CreateDirectoryA((projectPath + "\\System").c_str(), NULL);
    CreateDirectoryA((projectPath + "\\System\\Data").c_str(), NULL);
    CreateDirectoryA((projectPath + "\\System\\Data\\Interface").c_str(), NULL);
    
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_s(&timeinfo, &now);
    std::ostringstream timeStr;
    timeStr << std::put_time(&timeinfo, "%Y-%m-%d %H:%M:%S");
    
    std::ofstream projFile(projectPath + "\\project.json");
    if (!projFile.is_open()) {
        std::cout << "[ProjectManager] Failed to create project.json" << std::endl;
        return;
    }
    
    projFile << "{\n";
    projFile << "  \"name\": \"" << name << "\",\n";
    projFile << "  \"type\": \"" << type << "\",\n";
    projFile << "  \"version\": \"" << version << "\",\n";
    projFile << "  \"description\": \"" << desc << "\",\n";
    projFile << "  \"directory\": \"" << directory << "\"\n";
    projFile << "}\n";
    projFile.close();
    std::cout << "[ProjectManager] project.json created" << std::endl;
    
    std::ofstream metaFile(projectPath + "\\meta.json");
    if (!metaFile.is_open()) {
        std::cout << "[ProjectManager] Failed to create meta.json" << std::endl;
        return;
    }
    
    metaFile << "{\n";
    metaFile << "  \"created\": \"" << timeStr.str() << "\",\n";
    metaFile << "  \"modified\": \"" << timeStr.str() << "\",\n";
    metaFile << "  \"engine_version\": \"1.0.0\"\n";
    metaFile << "}\n";
    metaFile.close();
    std::cout << "[ProjectManager] meta.json created" << std::endl;
    
    // Загружаем созданный проект в AssetManager
    AssetManager::Instance().LoadProject(projectPath);
    
    MessageBoxA(NULL, ("Project created at:\n" + projectPath).c_str(), "Success", MB_OK);
    std::cout << "[ProjectManager] Project creation completed" << std::endl;
}