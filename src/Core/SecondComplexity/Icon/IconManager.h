#ifndef ICON_MANAGER_H
#define ICON_MANAGER_H

#include <string>
#include <map>
#include <vector>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <GL/glew.h>
#include "../../Render/Win32/RenderUI.h"

namespace fs = std::filesystem;

struct IconUV {
    float u1, v1, u2, v2;
    int width, height;
    GLuint textureId;
};

struct IconAtlas {
    std::string name;
    int iconSize;
    std::map<std::string, IconUV> uvMap;
    bool loaded;
    GLuint textureId;
};

class IconManager {
public:
    static IconManager& Instance() {
        static IconManager instance;
        return instance;
    }
    
    void SetIconDirectory(const std::string& dir) {
        iconDirectory = dir;
        LoadAtlases();
    }
    
    void SetRenderer(RenderUI* r) {
        renderer = r;
        ReloadTextures();
    }
    
    IconUV GetIconUV(const std::string& iconType, int size = 64) {
        std::string atlasName = GetAtlasNameForSize(size);
        
        auto atlasIt = atlases.find(atlasName);
        if (atlasIt == atlases.end()) {
            return GetDefaultIconUV(size);
        }
        
        IconAtlas& atlas = atlasIt->second;
        auto uvIt = atlas.uvMap.find(iconType);
        if (uvIt != atlas.uvMap.end()) {
            IconUV result = uvIt->second;
            result.textureId = atlas.textureId;
            return result;
        }
        
        uvIt = atlas.uvMap.find("unknown");
        if (uvIt != atlas.uvMap.end()) {
            IconUV result = uvIt->second;
            result.textureId = atlas.textureId;
            return result;
        }
        
        return GetDefaultIconUV(size);
    }
    
    bool HasIcon(const std::string& iconType, int size = 64) {
        std::string atlasName = GetAtlasNameForSize(size);
        auto atlasIt = atlases.find(atlasName);
        if (atlasIt == atlases.end()) return false;
        return atlasIt->second.uvMap.find(iconType) != atlasIt->second.uvMap.end();
    }
    
    std::vector<int> GetAvailableSizes() {
        std::vector<int> sizes;
        for (auto& [name, atlas] : atlases) {
            sizes.push_back(atlas.iconSize);
        }
        return sizes;
    }
    
    void ReloadTextures() {
        if (!renderer) return;
        
        for (auto& [name, atlas] : atlases) {
            std::string atlasPath = iconDirectory + "\\atlas_" + atlas.name + ".png";
            if (fs::exists(atlasPath)) {
                atlas.textureId = renderer->loadTextureFromFile(atlasPath);
                std::cout << "[IconManager] Loaded atlas texture: " << atlasPath << std::endl;
            }
        }
    }
    
private:
    IconManager() : renderer(nullptr) {
        atlases["16"] = {"16", 16, {}, false, 0};
        atlases["32"] = {"32", 32, {}, false, 0};
        atlases["64"] = {"64", 64, {}, false, 0};
        atlases["128"] = {"128", 128, {}, false, 0};
    }
    
    std::string iconDirectory = "System\\Data\\Interface";
    std::map<std::string, IconAtlas> atlases;
    RenderUI* renderer;
    
    std::string GetAtlasNameForSize(int size) {
        if (size <= 16) return "16";
        if (size <= 32) return "32";
        if (size <= 64) return "64";
        return "128";
    }
    
    void LoadAtlases() {
        for (auto& [name, atlas] : atlases) {
            LoadAtlasConfig(atlas);
        }
        ReloadTextures();
    }
    
    void LoadAtlasConfig(IconAtlas& atlas) {
        std::string configPath = iconDirectory + "\\atlas_" + atlas.name + ".json";
        
        if (!fs::exists(configPath)) {
            GenerateDefaultUVs(atlas);
            return;
        }
        
        std::ifstream file(configPath);
        if (!file.is_open()) {
            GenerateDefaultUVs(atlas);
            return;
        }
        
        std::string content((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());
        file.close();
        
        atlas.loaded = true;
        
        size_t gridSizePos = content.find("\"gridSize\"");
        int gridSize = 8;
        if (gridSizePos != std::string::npos) {
            size_t colon = content.find(":", gridSizePos);
            if (colon != std::string::npos) {
                size_t start = colon + 1;
                while (start < content.length() && (content[start] == ' ' || content[start] == '\t')) start++;
                gridSize = std::stoi(content.substr(start));
            }
        }
        
        size_t itemsPos = content.find("\"items\"");
        if (itemsPos == std::string::npos) {
            GenerateDefaultUVs(atlas);
            return;
        }
        
        size_t objStart = content.find("{", itemsPos);
        size_t objEnd = content.find("}", objStart);
        if (objStart == std::string::npos || objEnd == std::string::npos) {
            GenerateDefaultUVs(atlas);
            return;
        }
        
        std::string itemsStr = content.substr(objStart + 1, objEnd - objStart - 1);
        
        size_t pos = 0;
        float cellSize = 1.0f / gridSize;
        
        while (pos < itemsStr.length()) {
            size_t keyStart = itemsStr.find("\"", pos);
            if (keyStart == std::string::npos) break;
            size_t keyEnd = itemsStr.find("\"", keyStart + 1);
            if (keyEnd == std::string::npos) break;
            
            std::string iconType = itemsStr.substr(keyStart + 1, keyEnd - keyStart - 1);
            
            size_t colonPos = itemsStr.find(":", keyEnd);
            if (colonPos == std::string::npos) break;
            
            size_t valueStart = itemsStr.find_first_of("0123456789", colonPos);
            if (valueStart == std::string::npos) break;
            size_t valueEnd = itemsStr.find_first_of(",}", valueStart);
            if (valueEnd == std::string::npos) break;
            
            std::string indexStr = itemsStr.substr(valueStart, valueEnd - valueStart);
            int index = std::stoi(indexStr);
            
            int row = index / gridSize;
            int col = index % gridSize;
            
            IconUV uv;
            uv.u1 = col * cellSize;
            uv.v1 = row * cellSize;
            uv.u2 = (col + 1) * cellSize;
            uv.v2 = (row + 1) * cellSize;
            uv.width = atlas.iconSize;
            uv.height = atlas.iconSize;
            uv.textureId = 0;
            
            atlas.uvMap[iconType] = uv;
            
            pos = valueEnd + 1;
        }
        
        std::cout << "[IconManager] Loaded atlas " << atlas.name << " with " << atlas.uvMap.size() << " icons" << std::endl;
    }
    
    void GenerateDefaultUVs(IconAtlas& atlas) {
        atlas.loaded = true;
        
        std::vector<std::string> types = {
            "folder", "unknown", "txt", "cpp", "h", "hpp", "png", "jpg", "jpeg", 
            "bmp", "tga", "obj", "fbx", "gltf", "glb", "dae", "json", "xml", 
            "yaml", "yml", "md", "log", "dll", "exe", "lib", "zip", "rar", "7z", 
            "tar", "mp4", "avi", "mov", "wmv", "mp3", "wav", "ogg", "flac"
        };
        
        int gridSize = 8;
        float cellSize = 1.0f / gridSize;
        
        for (size_t i = 0; i < types.size() && i < (size_t)(gridSize * gridSize); i++) {
            int row = (int)i / gridSize;
            int col = (int)i % gridSize;
            
            IconUV uv;
            uv.u1 = col * cellSize;
            uv.v1 = row * cellSize;
            uv.u2 = (col + 1) * cellSize;
            uv.v2 = (row + 1) * cellSize;
            uv.width = atlas.iconSize;
            uv.height = atlas.iconSize;
            uv.textureId = 0;
            
            atlas.uvMap[types[i]] = uv;
        }
        
        std::cout << "[IconManager] Generated default UVs for atlas " << atlas.name << std::endl;
    }
    
    IconUV GetDefaultIconUV(int size) {
        IconUV uv;
        uv.u1 = 0.0f; uv.v1 = 0.0f;
        uv.u2 = 1.0f; uv.v2 = 1.0f;
        uv.width = size;
        uv.height = size;
        uv.textureId = 0;
        return uv;
    }
};

#endif