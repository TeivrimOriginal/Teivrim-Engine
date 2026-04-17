#ifndef ASSET_MANAGER_H
#define ASSET_MANAGER_H

#include <string>
#include <vector>
#include <functional>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <filesystem>

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
                for (auto& c : type) c = tolower(c);
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
    std::function<void()> onAssetsCleared;
    std::function<void(const std::string&)> onProjectLoaded;
    
    void LoadProject(const std::string& projectPath) {
        ClearAssets();
        projectRoot = projectPath;
        std::cout << "[AssetManager] Loading: " << projectPath << std::endl;
        
        tempPath = GetTempProjectPath(projectPath);
        CopyToTemp(projectPath, tempPath);
        rootAsset = ScanDirectory(tempPath, nullptr);
        
        if (onProjectLoaded) onProjectLoaded(projectPath);
    }
    
    void ClearAssets() {
        if (rootAsset) { delete rootAsset; rootAsset = nullptr; }
        flatAssetList.clear();
        if (onAssetsCleared) onAssetsCleared();
    }
    
    Asset* GetRootAsset() const { return rootAsset; }
    const std::vector<Asset*>& GetFlatAssetList() const { return flatAssetList; }
    std::string GetProjectRoot() const { return projectRoot; }  // ГЕТТЕР ДОБАВЛЕН СЮДА
    
    Asset* FindAssetByPath(const std::string& path) {
        for (auto a : flatAssetList) if (a->path == path) return a;
        return nullptr;
    }
    
private:
    AssetManager() : rootAsset(nullptr) {}
    ~AssetManager() { ClearAssets(); }
    
    Asset* rootAsset = nullptr;
    std::vector<Asset*> flatAssetList;
    std::string projectRoot;
    std::string tempPath;
    
    std::string GetTempProjectPath(const std::string& projectPath) {
        char tempDir[MAX_PATH];
        GetTempPathA(MAX_PATH, tempDir);
        fs::path projPath(projectPath);
        std::string tempProj = std::string(tempDir) + "GameEngine\\" + projPath.filename().string() + "\\";
        fs::create_directories(tempProj);
        return tempProj;
    }
    
    void CopyToTemp(const std::string& src, const std::string& dst) {
        try { fs::copy(src, dst, fs::copy_options::recursive | fs::copy_options::overwrite_existing); }
        catch (const std::exception& e) { std::cerr << "[AssetManager] Copy failed: " << e.what() << std::endl; }
    }
    
    Asset* ScanDirectory(const std::string& dirPath, Asset* parent) {
        fs::path dir(dirPath);
        Asset* dirAsset = new Asset(dir.filename().string(), dirPath, true);
        dirAsset->parent = parent;
        flatAssetList.push_back(dirAsset);
        
        try {
            for (const auto& entry : fs::directory_iterator(dirPath)) {
                if (fs::is_directory(entry)) {
                    dirAsset->children.push_back(ScanDirectory(entry.path().string(), dirAsset));
                } else {
                    Asset* fileAsset = new Asset(entry.path().filename().string(), entry.path().string(), false);
                    fileAsset->parent = dirAsset;
                    dirAsset->children.push_back(fileAsset);
                    flatAssetList.push_back(fileAsset);
                }
            }
        } catch (...) {}
        return dirAsset;
    }
};

#endif