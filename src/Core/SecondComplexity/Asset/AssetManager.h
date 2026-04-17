#ifndef ASSET_MANAGER_H
#define ASSET_MANAGER_H

#include <string>
#include <vector>
#include <functional>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <chrono>
#include <cctype>

namespace fs = std::filesystem;

class Asset {
public:
    std::string name;
    std::string path;
    std::string type;
    std::string extension;
    bool isFolder;
    std::vector<Asset*> children;
    Asset* parent;
    
    Asset(const std::string& _name, const std::string& _path, bool _isFolder = false)
        : name(_name), path(_path), isFolder(_isFolder), parent(nullptr) {
        if (!isFolder) {
            size_t dotPos = name.find_last_of('.');
            if (dotPos != std::string::npos) {
                extension = name.substr(dotPos);
                type = name.substr(dotPos + 1);
                for (auto& c : type) c = (char)tolower((unsigned char)c);
            } else {
                extension = "";
                type = "unknown";
            }
        } else {
            type = "folder";
            extension = "";
        }
    }
    
    ~Asset() {
        for (auto child : children) delete child;
    }
    
    std::string GetIconName() const { return type + ".svg"; }
};

class AssetManager {
public:
    static AssetManager& Instance() {
        static AssetManager instance;
        return instance;
    }
    
    std::function<void(Asset*)> onAssetAdded;
    std::function<void(Asset*)> onAssetRemoved;
    std::function<void()> onAssetsCleared;
    std::function<void(const std::string&)> onProjectLoaded;
    
    void LoadProject(const std::string& projectPath) {
        ClearAssets();
        projectRoot = projectPath;
        std::cout << "[AssetManager] Loading project: " << projectPath << std::endl;
        
        tempPath = GetTempProjectDirectory(projectPath);
        CopyToTemp(projectPath, tempPath);
        rootAsset = ScanDirectory(tempPath, nullptr);
        
        if (onProjectLoaded) onProjectLoaded(projectPath);
        std::cout << "[AssetManager] Project loaded, root: " << (rootAsset ? rootAsset->name : "null") << std::endl;
    }
    
    void ClearAssets() {
        if (rootAsset) { 
            delete rootAsset; 
            rootAsset = nullptr; 
        }
        flatAssetList.clear();
        if (onAssetsCleared) onAssetsCleared();
    }
    
    void Refresh() {
        if (!projectRoot.empty()) {
            LoadProject(projectRoot);
        }
    }
    
    Asset* GetRootAsset() const { return rootAsset; }
    const std::vector<Asset*>& GetFlatAssetList() const { return flatAssetList; }
    std::string GetProjectRoot() const { return projectRoot; }
    std::string GetTempDirectory() const { return tempPath; }
    
    Asset* FindAssetByPath(const std::string& path) {
        for (auto a : flatAssetList) if (a->path == path) return a;
        return nullptr;
    }
    
    Asset* FindAssetByName(const std::string& name) {
        for (auto a : flatAssetList) if (a->name == name) return a;
        return nullptr;
    }
    
    void CreateFolder(const std::string& folderName, Asset* parent = nullptr) {
        if (!parent) parent = rootAsset;
        if (!parent || !parent->isFolder) return;
        
        std::string folderPath = parent->path + "\\" + folderName;
        if (fs::create_directory(folderPath)) {
            Asset* newFolder = new Asset(folderName, folderPath, true);
            newFolder->parent = parent;
            parent->children.push_back(newFolder);
            flatAssetList.push_back(newFolder);
            
            if (onAssetAdded) onAssetAdded(newFolder);
            std::cout << "[AssetManager] Created folder: " << folderPath << std::endl;
        }
    }
    
    void DeleteAsset(Asset* asset) {
        if (!asset) return;
        
        try {
            if (asset->isFolder) {
                fs::remove_all(asset->path);
            } else {
                fs::remove(asset->path);
            }
            
            // Удаляем из parent
            if (asset->parent) {
                auto& children = asset->parent->children;
                children.erase(std::remove(children.begin(), children.end(), asset), children.end());
            }
            
            // Удаляем из flatAssetList
            flatAssetList.erase(std::remove(flatAssetList.begin(), flatAssetList.end(), asset), flatAssetList.end());
            
            if (onAssetRemoved) onAssetRemoved(asset);
            delete asset;
            std::cout << "[AssetManager] Deleted: " << asset->path << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[AssetManager] Failed to delete: " << e.what() << std::endl;
        }
    }
    
    bool RenameAsset(Asset* asset, const std::string& newName) {
        if (!asset) return false;
        
        try {
            fs::path oldPath(asset->path);
            fs::path newPath = oldPath.parent_path() / newName;
            
            fs::rename(oldPath, newPath);
            
            asset->name = newName;
            asset->path = newPath.string();
            
            if (!asset->isFolder) {
                size_t dotPos = newName.find_last_of('.');
                if (dotPos != std::string::npos) {
                    asset->extension = newName.substr(dotPos);
                    asset->type = newName.substr(dotPos + 1);
                    for (auto& c : asset->type) c = (char)tolower((unsigned char)c);
                }
            }
            
            std::cout << "[AssetManager] Renamed to: " << newName << std::endl;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "[AssetManager] Failed to rename: " << e.what() << std::endl;
            return false;
        }
    }
    
private:
    AssetManager() : rootAsset(nullptr) {}
    ~AssetManager() { ClearAssets(); }
    
    Asset* rootAsset = nullptr;
    std::vector<Asset*> flatAssetList;
    std::string projectRoot;
    std::string tempPath;
    
    std::string GetTempProjectDirectory(const std::string& projectPath) {
        char tempDir[MAX_PATH];
        DWORD result = ::GetTempPathA(MAX_PATH, tempDir);  // Явно вызываем WinAPI
        if (result == 0) {
            return "C:\\Temp\\GameEngine\\";
        }
        
        fs::path projPath(projectPath);
        std::string projName = projPath.filename().string();
        
        auto now = std::chrono::steady_clock::now().time_since_epoch().count();
        std::string tempProj = std::string(tempDir) + "GameEngine\\" + projName + "_" + 
                               std::to_string(now) + "\\";
        
        std::error_code ec;
        fs::create_directories(tempProj, ec);
        return tempProj;
    }
    
    void CopyToTemp(const std::string& src, const std::string& dst) {
        try { 
            std::error_code ec;
            fs::copy(src, dst, fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
            if (!ec) {
                std::cout << "[AssetManager] Copied to temp: " << dst << std::endl;
            }
        }
        catch (const std::exception& e) { 
            std::cerr << "[AssetManager] Copy failed: " << e.what() << std::endl; 
        }
    }
    
    Asset* ScanDirectory(const std::string& dirPath, Asset* parent) {
        fs::path dir(dirPath);
        Asset* dirAsset = new Asset(dir.filename().string(), dirPath, true);
        dirAsset->parent = parent;
        flatAssetList.push_back(dirAsset);
        
        try {
            for (const auto& entry : fs::directory_iterator(dirPath)) {
                if (fs::is_directory(entry)) {
                    Asset* subDir = ScanDirectory(entry.path().string(), dirAsset);
                    dirAsset->children.push_back(subDir);
                } else {
                    Asset* fileAsset = new Asset(entry.path().filename().string(), entry.path().string(), false);
                    fileAsset->parent = dirAsset;
                    dirAsset->children.push_back(fileAsset);
                    flatAssetList.push_back(fileAsset);
                    std::cout << "[AssetManager] Found: " << fileAsset->name << " (" << fileAsset->type << ")" << std::endl;
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "[AssetManager] Scan error: " << e.what() << std::endl;
        }
        
        return dirAsset;
    }
};

#endif