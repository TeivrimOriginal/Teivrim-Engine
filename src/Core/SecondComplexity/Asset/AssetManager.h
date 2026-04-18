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
#include <map>
#include <random>
#include <sstream>
#include <iomanip>

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
        if (projectRoot == projectPath && rootAsset != nullptr) {
            return;
        }
        
        ClearAssets();
        projectRoot = projectPath;
        originalProjectPath = projectPath;
        
        // Убираем конечный слэш если есть
        if (!originalProjectPath.empty() && originalProjectPath.back() == '\\') {
            originalProjectPath.pop_back();
        }
        if (!projectRoot.empty() && projectRoot.back() == '\\') {
            projectRoot.pop_back();
        }
        
        tempPath = GetTempProjectDirectory(projectPath);
        if (!tempPath.empty() && tempPath.back() == '\\') {
            tempPath.pop_back();
        }
        tempPath += "\\";
        
        std::cout << "[AssetManager] Original path: " << originalProjectPath << std::endl;
        std::cout << "[AssetManager] Temp path: " << tempPath << std::endl;
        
        if (!fs::exists(tempPath)) {
            fs::create_directories(tempPath);
            CopyToTemp(originalProjectPath, tempPath);
        }
        
        rootAsset = ScanDirectory(tempPath, nullptr);
        
        if (onProjectLoaded) onProjectLoaded(projectPath);
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
        if (tempPath.empty() || !fs::exists(tempPath)) return;
        ClearAssets();
        rootAsset = ScanDirectory(tempPath, nullptr);
    }
    
    void SaveProject() {
        if (originalProjectPath.empty() || tempPath.empty()) {
            std::cerr << "[AssetManager] Save failed - paths empty" << std::endl;
            return;
        }
        
        std::cout << "[AssetManager] Saving from: " << tempPath << std::endl;
        std::cout << "[AssetManager] Saving to: " << originalProjectPath << std::endl;
        
        try {
            // Удаляем из оригинала то, чего нет в темпе
            for (const auto& entry : fs::recursive_directory_iterator(originalProjectPath)) {
                std::string origPath = entry.path().string();
                std::string relPath = origPath.substr(originalProjectPath.length());
                if (!relPath.empty() && relPath[0] == '\\') relPath = relPath.substr(1);
                
                std::string tempFilePath = tempPath + "\\" + relPath;
                
                if (!fs::exists(tempFilePath)) {
                    std::cout << "[AssetManager] Removing: " << origPath << std::endl;
                    fs::remove_all(origPath);
                }
            }
            
            // Копируем из темпа в оригинал
            for (const auto& entry : fs::recursive_directory_iterator(tempPath)) {
                std::string tempFilePath = entry.path().string();
                std::string relPath = tempFilePath.substr(tempPath.length());
                if (!relPath.empty() && relPath[0] == '\\') relPath = relPath.substr(1);
                
                std::string originalFilePath = originalProjectPath + "\\" + relPath;
                
                if (fs::is_directory(entry)) {
                    if (!fs::exists(originalFilePath)) {
                        std::cout << "[AssetManager] Creating dir: " << originalFilePath << std::endl;
                        fs::create_directories(originalFilePath);
                    }
                } else {
                    bool needCopy = false;
                    if (!fs::exists(originalFilePath)) {
                        needCopy = true;
                    } else {
                        auto tempTime = fs::last_write_time(tempFilePath);
                        auto origTime = fs::last_write_time(originalFilePath);
                        if (tempTime > origTime) {
                            needCopy = true;
                        }
                    }
                    
                    if (needCopy) {
                        std::cout << "[AssetManager] Copying file: " << relPath << std::endl;
                        fs::copy_file(tempFilePath, originalFilePath, fs::copy_options::overwrite_existing);
                    }
                }
            }
            
            std::cout << "[AssetManager] Save completed!" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[AssetManager] Save error: " << e.what() << std::endl;
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
    
    void DeleteAsset(Asset* asset) {
        if (!asset) return;
        try {
            if (asset->isFolder) fs::remove_all(asset->path);
            else fs::remove(asset->path);
            Refresh();
        } catch (...) {}
    }
    
    bool RenameAsset(Asset* asset, const std::string& newName) {
        if (!asset) return false;
        try {
            fs::path oldPath(asset->path);
            fs::path newPath = oldPath.parent_path() / newName;
            fs::rename(oldPath, newPath);
            Refresh();
            return true;
        } catch (...) {
            return false;
        }
    }
    
    bool CreateFile(const std::string& filePath) {
        try {
            fs::path parentPath = fs::path(filePath).parent_path();
            if (!fs::exists(parentPath)) {
                fs::create_directories(parentPath);
            }
            std::ofstream file(filePath);
            file.close();
            Refresh();
            return true;
        } catch (...) {
            return false;
        }
    }
    
    bool CreateFolder(const std::string& folderPath) {
        try {
            if (!fs::exists(folderPath)) {
                fs::create_directories(folderPath);
                Refresh();
                return true;
            }
            return false;
        } catch (...) {
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
    std::string originalProjectPath;
    
    std::string GenerateUniqueId() {
        auto now = std::chrono::system_clock::now();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1000, 9999);
        std::ostringstream oss;
        oss << std::hex << millis << "_" << dis(gen);
        return oss.str();
    }
    
    std::string GetTempProjectDirectory(const std::string& projectPath) {
        char tempDir[MAX_PATH];
        ::GetTempPathA(MAX_PATH, tempDir);
        
        fs::path projPath(projectPath);
        std::string projName = projPath.filename().string();
        
        static std::string sessionId = GenerateUniqueId();
        std::string result = std::string(tempDir) + "GameEngine\\" + projName + "_" + sessionId;
        return result;
    }
    
    void CopyToTemp(const std::string& src, const std::string& dst) {
        try { 
            std::error_code ec;
            fs::copy(src, dst, fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
        } catch (...) {}
    }
    
    Asset* ScanDirectory(const std::string& dirPath, Asset* parent) {
        if (!fs::exists(dirPath)) return nullptr;
        
        fs::path dir(dirPath);
        std::string dirName = dir.filename().string();
        Asset* dirAsset = new Asset(dirName, dirPath, true);
        dirAsset->parent = parent;
        flatAssetList.push_back(dirAsset);
        
        try {
            for (const auto& entry : fs::directory_iterator(dirPath)) {
                if (fs::is_directory(entry)) {
                    Asset* subDir = ScanDirectory(entry.path().string(), dirAsset);
                    if (subDir) dirAsset->children.push_back(subDir);
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