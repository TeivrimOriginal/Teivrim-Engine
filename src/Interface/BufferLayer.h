#ifndef BUFFER_LAYER_H
#define BUFFER_LAYER_H

#include "../Core/SecondComplexity/Asset/AssetManager.h"
#include "../Core/Render/RenderUI.h"
#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <commdlg.h>
#include <fstream>
#include <chrono>

class InterfaceManager;
enum class RenderAPI;

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
    
    bool showContextMenu = false;
    int menuX = 0, menuY = 0;
    Asset* selectedAsset = nullptr;
    
    bool isRenaming = false;
    bool isCreating = false;
    bool creatingIsFolder = false;
    Asset* renamingAsset = nullptr;
    std::string renameBuffer;
    
    void VivodAsset(RenderUI& renderer, int startX, int startY, int areaW, int areaH, InterfaceManager* uiManager = nullptr) {
        Asset* root = AssetManager::Instance().GetRootAsset();
        if (!root) return;
        
        if (!currentDir) currentDir = root;
        
        std::vector<Asset*> assets = currentDir->children;
        gridItems.clear();
        
        int iconSize = 64, spacing = 10;
        int itemW = iconSize + spacing * 2, itemH = iconSize + spacing * 2 + 20;
        int cols = areaW / itemW; 
        if (cols < 1) cols = 1;
        
        int curX = startX, curY = startY, colIdx = 0;
        
        for (auto asset : assets) {
            GridItem item{asset, curX, curY, itemW, itemH};
            gridItems.push_back(item);
            
            renderer.drawQuad(curX, curY, curX + itemW, curY + itemH, 0.18f, 0.18f, 0.22f);
            
            int iconX = curX + spacing, iconY = curY + spacing;
            
            if (uiManager) {
                std::string iconType = asset->isFolder ? "folder" : asset->type;
                uiManager->printIcon(iconX, iconY, iconSize, iconSize, iconType, iconSize);
            } else {
                renderer.drawQuad(iconX, iconY, iconX + iconSize, iconY + iconSize, 0.5f, 0.5f, 0.5f);
            }
            
            if (!(isRenaming && renamingAsset == asset)) {
                std::string displayName = asset->name;
                if (displayName.length() > 10) displayName = displayName.substr(0, 7) + "...";
                int textX = curX + (itemW - (int)displayName.length() * 8) / 2;
                renderer.drawText(textX, curY + iconSize + spacing + 5, displayName, 0.9f, 0.9f, 0.9f);
            }
            
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
        
        if (isRenaming && renamingAsset) {
            for (auto& item : gridItems) {
                if (item.asset == renamingAsset) {
                    int inputW = item.w - 10;
                    int inputH = 20;
                    int inputX = item.x + 5;
                    int inputY = item.y + item.h - 25;
                    
                    renderer.drawQuad(inputX, inputY, inputX + inputW, inputY + inputH, 0.12f, 0.12f, 0.15f);
                    renderer.drawQuad(inputX, inputY, inputX + inputW, inputY + 1, 0.4f, 0.4f, 0.45f);
                    renderer.drawQuad(inputX, inputY + inputH - 1, inputX + inputW, inputY + inputH, 0.4f, 0.4f, 0.45f);
                    renderer.drawQuad(inputX, inputY, inputX + 1, inputY + inputH, 0.4f, 0.4f, 0.45f);
                    renderer.drawQuad(inputX + inputW - 1, inputY, inputX + inputW, inputY + inputH, 0.4f, 0.4f, 0.45f);
                    
                    renderer.drawText(inputX + 5, inputY + 4, renameBuffer + "_", 1.0f, 1.0f, 1.0f);
                    break;
                }
            }
        }
        
        if (isCreating) {
            int createX = curX, createY = curY;
            renderer.drawQuad(createX, createY, createX + itemW, createY + itemH, 0.18f, 0.18f, 0.22f);
            
            int inputW = itemW - 10, inputH = 20;
            int inputX = createX + 5, inputY = createY + itemH - 25;
            
            renderer.drawQuad(inputX, inputY, inputX + inputW, inputY + inputH, 0.12f, 0.12f, 0.15f);
            renderer.drawQuad(inputX, inputY, inputX + inputW, inputY + 1, 0.4f, 0.4f, 0.45f);
            renderer.drawQuad(inputX, inputY + inputH - 1, inputX + inputW, inputY + inputH, 0.4f, 0.4f, 0.45f);
            renderer.drawQuad(inputX, inputY, inputX + 1, inputY + inputH, 0.4f, 0.4f, 0.45f);
            renderer.drawQuad(inputX + inputW - 1, inputY, inputX + inputW, inputY + inputH, 0.4f, 0.4f, 0.45f);
            
            renderer.drawText(inputX + 5, inputY + 4, renameBuffer + "_", 1.0f, 1.0f, 1.0f);
        }
        
        if (showContextMenu) {
            int menuW = 140, menuItemH = 24;
            std::vector<std::string> items;
            
            if (selectedAsset) {
                if (selectedAsset->isFolder) {
                    items = {"Open", "Rename", "Delete", "Info"};
                } else {
                    items = {"Open", "Rename", "Delete", "Info"};
                }
            } else {
                items = {"Create File", "Create Folder", "Refresh", "Info"};
            }
            
            int menuH = (int)items.size() * menuItemH;
            
            RECT clientRect;
            GetClientRect(parentHwnd, &clientRect);
            if (menuX + menuW > clientRect.right) menuX = clientRect.right - menuW - 5;
            if (menuY + menuH > clientRect.bottom) menuY = clientRect.bottom - menuH - 5;
            if (menuX < 5) menuX = 5;
            if (menuY < 5) menuY = 5;
            
            renderer.drawQuad(menuX, menuY, menuX + menuW, menuY + menuH, 0.12f, 0.12f, 0.15f);
            
            for (size_t i = 0; i < items.size(); i++) {
                int itemY = menuY + i * menuItemH;
                renderer.drawQuad(menuX + 2, itemY + 2, menuX + menuW - 2, itemY + menuItemH - 2, 0.20f, 0.20f, 0.24f);
                renderer.drawText(menuX + 10, itemY + 6, items[i], 0.95f, 0.95f, 0.95f);
            }
        }
    }
    
    void HandleContextMenuClick(int mouseX, int mouseY) {
        if (!showContextMenu) return;
        
        int menuW = 140, menuItemH = 24;
        int itemCount = selectedAsset ? 4 : 4;
        int menuH = itemCount * menuItemH;
        
        if (mouseX < menuX || mouseX > menuX + menuW || mouseY < menuY || mouseY > menuY + menuH) {
            showContextMenu = false;
            return;
        }
        
        int itemIndex = (mouseY - menuY) / menuItemH;
        
        if (selectedAsset) {
            if (selectedAsset->isFolder) {
                switch (itemIndex) {
                    case 0: NavigateTo(selectedAsset); break;
                    case 1: StartRenaming(selectedAsset); break;
                    case 2: DeleteAssetDialog(selectedAsset); break;
                    case 3: ShowAssetInfo(selectedAsset); break;
                }
            } else {
                switch (itemIndex) {
                    case 0: OpenAssetExternal(selectedAsset); break;
                    case 1: StartRenaming(selectedAsset); break;
                    case 2: DeleteAssetDialog(selectedAsset); break;
                    case 3: ShowAssetInfo(selectedAsset); break;
                }
            }
        } else {
            switch (itemIndex) {
                case 0: StartCreatingFile(); break;
                case 1: StartCreatingFolder(); break;
                case 2: AssetManager::Instance().Refresh(); ResetNavigation(); break;
                case 3: ShowFolderInfo(); break;
            }
        }
        
        showContextMenu = false;
    }
    
    void StartRenaming(Asset* asset) {
        if (!asset) return;
        isRenaming = true;
        isCreating = false;
        renamingAsset = asset;
        renameBuffer = asset->name;
        showContextMenu = false;
    }
    
    void StartCreatingFile() {
        isCreating = true;
        isRenaming = false;
        creatingIsFolder = false;
        renameBuffer = "NewFile.txt";
        showContextMenu = false;
    }
    
    void StartCreatingFolder() {
        isCreating = true;
        isRenaming = false;
        creatingIsFolder = true;
        renameBuffer = "NewFolder";
        showContextMenu = false;
    }
    
    void HandleKeyboardInput(WPARAM wParam) {
        if (!isRenaming && !isCreating) return;
        
        if (wParam == VK_RETURN) {
            if (isRenaming && renamingAsset) {
                if (!renameBuffer.empty() && renameBuffer != renamingAsset->name) {
                    AssetManager::Instance().RenameAsset(renamingAsset, renameBuffer);
                }
            } else if (isCreating) {
                if (!renameBuffer.empty()) {
                    std::string newPath = currentDir->path + "\\" + renameBuffer;
                    if (creatingIsFolder) {
                        CreateDirectoryA(newPath.c_str(), NULL);
                    } else {
                        std::ofstream file(newPath);
                        file.close();
                    }
                    AssetManager::Instance().Refresh();
                    currentDir = AssetManager::Instance().FindAssetByPath(currentDir->path);
                    if (!currentDir) currentDir = AssetManager::Instance().GetRootAsset();
                }
            }
            isRenaming = false;
            isCreating = false;
            renamingAsset = nullptr;
        } else if (wParam == VK_ESCAPE) {
            isRenaming = false;
            isCreating = false;
            renamingAsset = nullptr;
        } else if (wParam == VK_BACK) {
            if (!renameBuffer.empty()) renameBuffer.pop_back();
        }
    }
    
    void HandleCharInput(char c) {
        if (!isRenaming && !isCreating) return;
        if (c >= 32 && c <= 126) renameBuffer += c;
    }
    
    void OpenAssetExternal(Asset* asset) {
        if (!asset || asset->isFolder) return;
        ShellExecuteA(NULL, "open", asset->path.c_str(), NULL, NULL, SW_SHOW);
    }
    
    void DeleteAssetDialog(Asset* asset) {
        if (!asset) return;
        std::string msg = "Delete '" + asset->name + "'?";
        if (MessageBoxA(parentHwnd, msg.c_str(), "Delete", MB_YESNO | MB_ICONQUESTION) == IDYES) {
            AssetManager::Instance().DeleteAsset(asset);
            currentDir = AssetManager::Instance().FindAssetByPath(currentDir->path);
            if (!currentDir) currentDir = AssetManager::Instance().GetRootAsset();
        }
    }
    
    void ShowAssetInfo(Asset* asset) {
        if (!asset) return;
        std::string info = "Name: " + asset->name + "\nType: " + asset->type + "\nPath: " + asset->path;
        MessageBoxA(parentHwnd, info.c_str(), "Asset Info", MB_OK);
    }
    
    void ShowFolderInfo() {
        if (!currentDir) return;
        int fileCount = 0, folderCount = 0;
        for (auto child : currentDir->children) {
            if (child->isFolder) folderCount++;
            else fileCount++;
        }
        std::string info = "Folder: " + currentDir->name + "\nFolders: " + std::to_string(folderCount) + "\nFiles: " + std::to_string(fileCount);
        MessageBoxA(parentHwnd, info.c_str(), "Folder Info", MB_OK);
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
    BufferLayer() : currentDir(nullptr), iconDirectory("System\\Data\\Interface"), parentHwnd(nullptr),
                    showContextMenu(false), selectedAsset(nullptr), isRenaming(false), isCreating(false),
                    renamingAsset(nullptr), creatingIsFolder(false) {}
    
    std::string iconDirectory;
    std::vector<GridItem> gridItems;
    Asset* currentDir;
    std::vector<Asset*> navStack;
    HWND parentHwnd;
};

#endif