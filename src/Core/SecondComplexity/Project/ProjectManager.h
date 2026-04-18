#ifndef PROJECT_MANAGER_H
#define PROJECT_MANAGER_H

#include <windows.h>
#include <string>
#include <functional>

class ProjectManager {
public:
    static ProjectManager& Instance() {
        static ProjectManager instance;
        return instance;
    }
    
    void SetRenderAPI(int api) { currentAPI = api; }
    
    void ShowCreateProjectDialog(HWND parent, std::function<void(const std::string&)> onCreated = nullptr);
    void CreateProject(const std::string& name, const std::string& type, const std::string& version,
                       const std::string& desc, const std::string& directory);

private:
    ProjectManager() : currentAPI(0) {}
    int currentAPI;
    
    struct DialogData {
        HWND nameEdit, typeEdit, versionEdit, descEdit, dirEdit;
        std::function<void(const std::string&)> callback;
    };
    
    static LRESULT CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

#endif