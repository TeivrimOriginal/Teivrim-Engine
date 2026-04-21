#include "CoreUtils.h"

namespace CoreUtils {

    WindowDimensions GetClientAreaDimensions(HWND hwnd) {
        RECT rect;
        GetClientRect(hwnd, &rect);
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;
        
        WindowDimensions dims(width, height);
        dims.validate();
        return dims;
    }

    bool OpenModelFileDialog(HWND hwnd, std::string& outPath) {
        OPENFILENAMEA ofn = {0};
        CHAR szFile[MAX_PATH] = "";
        
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hwnd;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = "3D Models\0*.obj;*.fbx;*.dae;*.gltf;*.glb\0";
        ofn.lpstrTitle = "Select 3D Model";
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
        
        if (GetOpenFileNameA(&ofn)) {
            outPath = ofn.lpstrFile;
            return true;
        }
        return false;
    }

    bool OpenProjectFileDialog(HWND hwnd, std::string& outPath) {
        OPENFILENAMEA ofn = {0};
        CHAR szFile[MAX_PATH] = "";
        
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hwnd;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = "Project Files (*.json)\0*.json\0";
        ofn.lpstrTitle = "Open Project";
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
        
        if (GetOpenFileNameA(&ofn)) {
            outPath = ofn.lpstrFile;
            size_t pos = outPath.find_last_of("\\/");
            if (pos != std::string::npos) {
                outPath = outPath.substr(0, pos);
            }
            return true;
        }
        return false;
    }

    POINT GetClientMousePosition(HWND hwnd) {
        POINT pos;
        GetCursorPos(&pos);
        ScreenToClient(hwnd, &pos);
        return pos;
    }

    void SetWindowTitleWithFPS(HWND hwnd, const char* apiName, int fps) {
        char title[256];
        sprintf_s(title, "%s 3D Viewer | FPS: %d", apiName, fps);
        SetWindowTextA(hwnd, title);
    }
}
