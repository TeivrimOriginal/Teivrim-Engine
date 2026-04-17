#ifndef BUFFER_LAYER_H
#define BUFFER_LAYER_H

#include "../Core/SecondComplexity/Asset/AssetManager.h"
#include "../Core/Render/Win32/RenderUI.h"
#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include <windows.h>
#include <shellapi.h>
#include <chrono>
#include <shlobj.h>      // ДОБАВИТЬ для BROWSEINFOA
#include <commdlg.h>     // ДОБАВИТЬ для OPENFILENAMEA

// ... остальной код без изменений ...
namespace fs = std::filesystem;

struct GridItem {
    Asset* asset;
    int x, y, w, h;
};

class BufferLayer {
public:
    static BufferLayer& Instance() {
        static BufferLayer instance;
        return instance;
    }
    
    void SetIconDirectory(const std::string& dir) { iconDirectory = dir; }
    void SetParentHWND(HWND hwnd) { parentHwnd = hwnd; }
    
    // Контекстное меню
    bool showContextMenu = false;
    int menuX = 0, menuY = 0;
    Asset* selectedAsset = nullptr;
    
    void VivodAsset(RenderUI& renderer, int startX, int startY, int areaW, int areaH) {
        Asset* root = AssetManager::Instance().GetRootAsset();
        if (!root) { 
            renderer.drawText(startX, startY, "No project loaded", 1.0f, 0.5f, 0.5f); 
            return; 
        }
        
        if (!currentDir) currentDir = root;
        
        std::vector<Asset*> assets = currentDir->children;
        gridItems.clear();
        
        if (assets.empty()) { 
            renderer.drawText(startX, startY, "Empty folder", 0.7f, 0.7f, 0.7f); 
        } else {
            int iconSize = 64, spacing = 10;
            int itemW = iconSize + spacing * 2, itemH = iconSize + spacing * 2 + 20;
            int cols = areaW / itemW; 
            if (cols < 1) cols = 1;
            
            int curX = startX, curY = startY, colIdx = 0;
            
            for (auto asset : assets) {
                GridItem item{asset, curX, curY, itemW, itemH};
                gridItems.push_back(item);
                
                // Фон элемента
                renderer.drawQuad(curX, curY, curX + itemW, curY + itemH, 0.18f, 0.18f, 0.22f);
                
                // Иконка
                int iconX = curX + spacing, iconY = curY + spacing;
                float r, g, b;
                GetColorForType(asset->type, r, g, b);
                renderer.drawQuad(iconX, iconY, iconX + iconSize, iconY + iconSize, r, g, b);
                
                // Имя файла
                std::string displayName = asset->name;
                if (displayName.length() > 10) displayName = displayName.substr(0, 7) + "...";
                int textX = curX + (itemW - (int)displayName.length() * 8) / 2;
                renderer.drawText(textX, curY + iconSize + spacing + 5, displayName, 0.9f, 0.9f, 0.9f);
                
                // Индикатор что папка не пустая
                if (asset->isFolder && !asset->children.empty()) {
                    renderer.drawText(iconX + iconSize - 15, iconY + 5, ">", 0.7f, 0.7f, 0.7f);
                }
                
                colIdx++;
                curX += itemW;
                if (colIdx >= cols) { 
                    colIdx = 0; 
                    curX = startX; 
                    curY += itemH; 
                }
            }
        }
        
        // Рендерим контекстное меню
        if (showContextMenu) {
            RenderContextMenu(renderer);
        }
    }
    
    void RenderContextMenu(RenderUI& renderer) {
        int menuW = 140;
        int menuItemH = 24;
        std::vector<std::string> items;
        
        if (selectedAsset) {
            if (selectedAsset->isFolder) {
                items = {"Open", "Rename", "Delete", "Info"};
            } else {
                items = {"Open", "Open with...", "Rename", "Delete", "Info"};
            }
        } else {
            items = {"Add File...", "Add Folder...", "Refresh", "Info"};
        }
        
        int menuH = (int)items.size() * menuItemH;
        
        // Проверка границ экрана
        RECT clientRect;
        GetClientRect(parentHwnd, &clientRect);
        if (menuX + menuW > clientRect.right) menuX = clientRect.right - menuW - 5;
        if (menuY + menuH > clientRect.bottom) menuY = clientRect.bottom - menuH - 5;
        if (menuX < 5) menuX = 5;
        if (menuY < 5) menuY = 5;
        
        // Фон меню
        renderer.drawQuad(menuX, menuY, menuX + menuW, menuY + menuH, 0.12f, 0.12f, 0.15f);
        
        // Рамка
        renderer.drawQuad(menuX, menuY, menuX + menuW, menuY + 1, 0.35f, 0.35f, 0.40f);
        renderer.drawQuad(menuX, menuY + menuH - 1, menuX + menuW, menuY + menuH, 0.35f, 0.35f, 0.40f);
        renderer.drawQuad(menuX, menuY, menuX + 1, menuY + menuH, 0.35f, 0.35f, 0.40f);
        renderer.drawQuad(menuX + menuW - 1, menuY, menuX + menuW, menuY + menuH, 0.35f, 0.35f, 0.40f);
        
        // Пункты меню
        for (size_t i = 0; i < items.size(); i++) {
            int itemY = menuY + i * menuItemH;
            renderer.drawQuad(menuX + 2, itemY + 2, menuX + menuW - 2, itemY + menuItemH - 2, 0.20f, 0.20f, 0.24f);
            renderer.drawText(menuX + 10, itemY + 6, items[i], 0.95f, 0.95f, 0.95f);
            
            // Разделитель
            if (i < items.size() - 1) {
                renderer.drawQuad(menuX + 5, itemY + menuItemH - 1, menuX + menuW - 5, itemY + menuItemH, 0.30f, 0.30f, 0.35f);
            }
        }
    }
    
    void HandleContextMenuClick(int mouseX, int mouseY) {
        if (!showContextMenu) return;
        
        int menuW = 140;
        int menuItemH = 24;
        int itemCount = selectedAsset ? (selectedAsset->isFolder ? 4 : 5) : 4;
        int menuH = itemCount * menuItemH;
        
        // Проверяем клик вне меню
        if (mouseX < menuX || mouseX > menuX + menuW || mouseY < menuY || mouseY > menuY + menuH) {
            showContextMenu = false;
            return;
        }
        
        int itemIndex = (mouseY - menuY) / menuItemH;
        
        if (selectedAsset) {
            if (selectedAsset->isFolder) {
                switch (itemIndex) {
                    case 0: NavigateTo(selectedAsset); break;
                    case 1: RenameAssetDialog(selectedAsset); break;
                    case 2: DeleteAssetDialog(selectedAsset); break;
                    case 3: ShowAssetInfo(selectedAsset); break;
                }
            } else {
                switch (itemIndex) {
                    case 0: OpenAssetExternal(selectedAsset); break;
                    case 1: OpenWithDialog(selectedAsset); break;
                    case 2: RenameAssetDialog(selectedAsset); break;
                    case 3: DeleteAssetDialog(selectedAsset); break;
                    case 4: ShowAssetInfo(selectedAsset); break;
                }
            }
        } else {
            switch (itemIndex) {
                case 0: AddFileDialog(); break;
                case 1: AddFolderDialog(); break;
                case 2: AssetManager::Instance().Refresh(); ResetNavigation(); break;
                case 3: ShowFolderInfo(); break;
            }
        }
        
        showContextMenu = false;
    }
    
    void OpenAssetExternal(Asset* asset) {
        if (!asset || asset->isFolder) return;
        ShellExecuteA(NULL, "open", asset->path.c_str(), NULL, NULL, SW_SHOW);
    }
    
    void OpenWithDialog(Asset* asset) {
        if (!asset || asset->isFolder) return;
        ShellExecuteA(NULL, "openas", asset->path.c_str(), NULL, NULL, SW_SHOW);
    }
    
    void DeleteAssetDialog(Asset* asset) {
        if (!asset) return;
        
        std::string msg = "Are you sure you want to delete '" + asset->name + "'?";
        int result = MessageBoxA(parentHwnd, msg.c_str(), "Delete", MB_YESNO | MB_ICONQUESTION);
        
        if (result == IDYES) {
            AssetManager::Instance().DeleteAsset(asset);
            ResetNavigation();
        }
    }
    
    void RenameAssetDialog(Asset* asset) {
        if (!asset) return;
        
        // Простой диалог через InputBox
        char newName[256] = {0};
        strcpy_s(newName, asset->name.c_str());
        
        if (InputBox("Rename", "Enter new name:", newName, 256)) {
            if (strlen(newName) > 0 && strcmp(newName, asset->name.c_str()) != 0) {
                AssetManager::Instance().RenameAsset(asset, newName);
                ResetNavigation();
            }
        }
    }
    
    void ShowAssetInfo(Asset* asset) {
        if (!asset) return;
        
        std::string info = "Name: " + asset->name + "\n";
        info += "Type: " + asset->type + "\n";
        info += "Path: " + asset->path + "\n";
        
        if (!asset->isFolder) {
            try {
                auto size = fs::file_size(asset->path);
                info += "Size: " + FormatSize(size) + "\n";
            } catch (...) {
                info += "Size: Unknown\n";
            }
        } else {
            info += "Items: " + std::to_string(asset->children.size()) + "\n";
        }
        
        MessageBoxA(parentHwnd, info.c_str(), "Asset Info", MB_OK | MB_ICONINFORMATION);
    }
    
    void ShowFolderInfo() {
        if (!currentDir) return;
        
        int fileCount = 0, folderCount = 0;
        uintmax_t totalSize = 0;
        
        for (auto child : currentDir->children) {
            if (child->isFolder) {
                folderCount++;
            } else {
                fileCount++;
                try {
                    totalSize += fs::file_size(child->path);
                } catch (...) {}
            }
        }
        
        std::string info = "Folder: " + currentDir->name + "\n";
        info += "Path: " + currentDir->path + "\n\n";
        info += "Folders: " + std::to_string(folderCount) + "\n";
        info += "Files: " + std::to_string(fileCount) + "\n";
        info += "Total items: " + std::to_string(currentDir->children.size()) + "\n";
        if (totalSize > 0) {
            info += "Total size: " + FormatSize(totalSize) + "\n";
        }
        
        MessageBoxA(parentHwnd, info.c_str(), "Folder Info", MB_OK | MB_ICONINFORMATION);
    }
    
    void AddFileDialog() {
        OPENFILENAMEA ofn;
        CHAR szFile[MAX_PATH] = "";
        
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = parentHwnd;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = "All Files\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT;
        ofn.lpstrTitle = "Select files to add";
        
        if (GetOpenFileNameA(&ofn)) {
            std::string src = ofn.lpstrFile;
            std::string dst = currentDir->path + "\\" + fs::path(src).filename().string();
            
            try {
                fs::copy(src, dst);
                AssetManager::Instance().Refresh();
                ResetNavigation();
            } catch (...) {
                MessageBoxA(parentHwnd, "Failed to add file", "Error", MB_OK);
            }
        }
    }
    
    void AddFolderDialog() {
        BROWSEINFOA bi = {0};
        bi.hwndOwner = parentHwnd;
        bi.lpszTitle = "Select folder to add";
        bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
        LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
        
        if (pidl) {
            char path[MAX_PATH];
            SHGetPathFromIDListA(pidl, path);
            
            std::string src = path;
            std::string dst = currentDir->path + "\\" + fs::path(src).filename().string();
            
            try {
                fs::copy(src, dst, fs::copy_options::recursive);
                AssetManager::Instance().Refresh();
                ResetNavigation();
            } catch (...) {
                MessageBoxA(parentHwnd, "Failed to add folder", "Error", MB_OK);
            }
            
            CoTaskMemFree(pidl);
        }
    }
    
    bool InputBox(const char* title, const char* prompt, char* buffer, int bufferSize) {
        // Упрощённая реализация через диалог
        std::string msg = std::string(prompt) + "\n\nCurrent: " + buffer;
        int result = MessageBoxA(parentHwnd, msg.c_str(), title, MB_OKCANCEL | MB_ICONQUESTION);
        
        if (result == IDOK) {
            // В реальном проекте здесь должен быть кастомный диалог с Edit Control
            // Пока просто показываем что функция вызвана
            MessageBoxA(parentHwnd, "Input dialog not implemented yet", "Info", MB_OK);
        }
        return false;
    }
    
    std::string FormatSize(uintmax_t bytes) {
        const char* units[] = {"B", "KB", "MB", "GB", "TB"};
        int unitIndex = 0;
        double size = (double)bytes;
        
        while (size >= 1024.0 && unitIndex < 4) {
            size /= 1024.0;
            unitIndex++;
        }
        
        char buffer[64];
        sprintf_s(buffer, "%.2f %s", size, units[unitIndex]);
        return std::string(buffer);
    }
    
    void SetCurrentDirectory(Asset* dir) { 
        if (dir && dir->isFolder) {
            if (currentDir) navStack.push_back(currentDir);
            currentDir = dir;
        }
    }
    
    Asset* GetCurrentDirectory() const { 
        return currentDir ? currentDir : AssetManager::Instance().GetRootAsset(); 
    }
    
    Asset* GetAssetAtPosition(int mouseX, int mouseY) {
        for (auto& item : gridItems) {
            if (mouseX >= item.x && mouseX <= item.x + item.w &&
                mouseY >= item.y && mouseY <= item.y + item.h) {
                return item.asset;
            }
        }
        return nullptr;
    }
    
    void NavigateBack() {
        if (!navStack.empty()) {
            currentDir = navStack.back();
            navStack.pop_back();
        }
    }
    
    void NavigateTo(Asset* folder) {
        if (!folder || !folder->isFolder) return;
        if (currentDir) navStack.push_back(currentDir);
        currentDir = folder;
    }
    
    void NavigateUp() {
        if (currentDir && currentDir->parent) {
            currentDir = currentDir->parent;
        }
    }
    
    void ResetNavigation() {
        currentDir = AssetManager::Instance().GetRootAsset();
        navStack.clear();
    }
    
private:
    BufferLayer() : currentDir(nullptr), iconDirectory("System\\Data\\Interface"), parentHwnd(nullptr) {}
    
    std::string iconDirectory;
    std::vector<GridItem> gridItems;
    Asset* currentDir;
    std::vector<Asset*> navStack;
    HWND parentHwnd;
    
    void GetColorForType(const std::string& type, float& r, float& g, float& b) {
        if (type == "folder") { 
            r = 0.9f; g = 0.75f; b = 0.3f; 
        } else if (type == "cpp" || type == "h" || type == "hpp") { 
            r = 0.3f; g = 0.5f; b = 0.9f; 
        } else if (type == "png" || type == "jpg" || type == "jpeg" || type == "bmp" || type == "tga") { 
            r = 0.3f; g = 0.8f; b = 0.4f; 
        } else if (type == "obj" || type == "fbx" || type == "gltf" || type == "glb" || type == "dae") { 
            r = 0.8f; g = 0.4f; b = 0.3f; 
        } else if (type == "json" || type == "xml" || type == "yaml" || type == "yml") { 
            r = 0.8f; g = 0.8f; b = 0.3f; 
        } else if (type == "txt" || type == "md" || type == "log") { 
            r = 0.7f; g = 0.7f; b = 0.7f; 
        } else if (type == "dll" || type == "exe" || type == "lib") { 
            r = 0.6f; g = 0.6f; b = 0.8f; 
        } else if (type == "zip" || type == "rar" || type == "7z" || type == "tar") { 
            r = 0.7f; g = 0.5f; b = 0.3f; 
        } else if (type == "mp4" || type == "avi" || type == "mov" || type == "wmv") { 
            r = 0.8f; g = 0.3f; b = 0.5f; 
        } else if (type == "mp3" || type == "wav" || type == "ogg" || type == "flac") { 
            r = 0.5f; g = 0.8f; b = 0.3f; 
        } else {
            size_t hash = std::hash<std::string>{}(type);
            r = ((hash >> 0) & 0xFF) / 255.0f * 0.4f + 0.3f;
            g = ((hash >> 8) & 0xFF) / 255.0f * 0.4f + 0.3f;
            b = ((hash >> 16) & 0xFF) / 255.0f * 0.4f + 0.3f;
        }
    }
};

#endif