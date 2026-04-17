#ifndef BUFFER_LAYER_H
#define BUFFER_LAYER_H

#include "../Core/SecondComplexity/Asset/AssetManager.h"
#include "../Core/Render/Win32/RenderUI.h"
#include <string>
#include <vector>
#include <map>
#include <filesystem>

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
    
    void VivodAsset(RenderUI& renderer, int startX, int startY, int areaW, int areaH) {
        Asset* root = AssetManager::Instance().GetRootAsset();
        if (!root) { 
            renderer.drawText(startX, startY, "No project loaded", 1.0f, 0.5f, 0.5f); 
            return; 
        }
        
        if (!currentDir) currentDir = root;
        
        std::vector<Asset*> assets = currentDir->children;
        if (assets.empty()) { 
            renderer.drawText(startX, startY, "Empty folder", 0.7f, 0.7f, 0.7f); 
            return; 
        }
        
        gridItems.clear();
        int iconSize = 64, spacing = 10;
        int itemW = iconSize + spacing * 2, itemH = iconSize + spacing * 2 + 20;
        int cols = areaW / itemW; 
        if (cols < 1) cols = 1;
        
        int curX = startX, curY = startY, colIdx = 0;
        
        for (auto asset : assets) {
            GridItem item{asset, curX, curY, itemW, itemH};
            gridItems.push_back(item);
            
            // Фон элемента
            renderer.drawQuad(curX, curY, curX + itemW, curY + itemH, 0.2f, 0.2f, 0.25f);
            
            // Иконка
            int iconX = curX + spacing, iconY = curY + spacing;
            float r, g, b;
            GetColorForType(asset->type, r, g, b);
            renderer.drawQuad(iconX, iconY, iconX + iconSize, iconY + iconSize, r, g, b);
            
            // Текст (имя файла)
            std::string displayName = asset->name;
            if (displayName.length() > 10) displayName = displayName.substr(0, 7) + "...";
            int textX = curX + (itemW - (int)displayName.length() * 8) / 2;
            renderer.drawText(textX, curY + iconSize + spacing + 5, displayName, 0.9f, 0.9f, 0.9f);
            
            // Индикатор папки
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
    BufferLayer() : currentDir(nullptr), iconDirectory("System\\Data\\Interface") {}
    
    std::string iconDirectory;
    std::vector<GridItem> gridItems;
    Asset* currentDir;
    std::vector<Asset*> navStack;
    
    void GetColorForType(const std::string& type, float& r, float& g, float& b) {
        size_t hash = std::hash<std::string>{}(type);
        r = ((hash >> 0) & 0xFF) / 255.0f * 0.5f + 0.3f;
        g = ((hash >> 8) & 0xFF) / 255.0f * 0.5f + 0.3f;
        b = ((hash >> 16) & 0xFF) / 255.0f * 0.5f + 0.3f;
        
        if (type == "folder") { r = 0.9f; g = 0.8f; b = 0.3f; }
        else if (type == "cpp" || type == "h") { r = 0.3f; g = 0.5f; b = 0.9f; }
        else if (type == "png" || type == "jpg" || type == "jpeg") { r = 0.3f; g = 0.8f; b = 0.4f; }
        else if (type == "obj" || type == "fbx" || type == "gltf" || type == "glb") { r = 0.8f; g = 0.4f; b = 0.3f; }
        else if (type == "json") { r = 0.8f; g = 0.8f; b = 0.3f; }
        else if (type == "txt") { r = 0.7f; g = 0.7f; b = 0.7f; }
    }
};

#endif