#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include <glm/glm.hpp>

struct StandardVertex {
    float position[3];
    float normal[3];
    float texCoords[2];
};

// Сырые данные текстуры (не зависит от API)
struct RawTextureData {
    unsigned char* data;
    int width;
    int height;
    int channels;
    std::string path;
    bool isValid;
    
    RawTextureData() : data(nullptr), width(0), height(0), channels(0), isValid(false) {}
    
    ~RawTextureData() { 
        if (data) delete[] data; 
    }
    
    // Запрещаем копирование
    RawTextureData(const RawTextureData&) = delete;
    RawTextureData& operator=(const RawTextureData&) = delete;
    
    // Разрешаем перемещение
    RawTextureData(RawTextureData&& other) noexcept 
        : data(other.data), width(other.width), height(other.height), 
          channels(other.channels), path(std::move(other.path)), isValid(other.isValid) {
        other.data = nullptr;
        other.isValid = false;
    }
    
    RawTextureData& operator=(RawTextureData&& other) noexcept {
        if (this != &other) {
            if (data) delete[] data;
            data = other.data;
            width = other.width;
            height = other.height;
            channels = other.channels;
            path = std::move(other.path);
            isValid = other.isValid;
            other.data = nullptr;
            other.isValid = false;
        }
        return *this;
    }
};

struct TextureData {
    std::string type;
    RawTextureData rawData;
    
    TextureData() = default;
    
    // Запрещаем копирование
    TextureData(const TextureData&) = delete;
    TextureData& operator=(const TextureData&) = delete;
    
    // Разрешаем перемещение
    TextureData(TextureData&& other) noexcept 
        : type(std::move(other.type)), rawData(std::move(other.rawData)) {}
    
    TextureData& operator=(TextureData&& other) noexcept {
        if (this != &other) {
            type = std::move(other.type);
            rawData = std::move(other.rawData);
        }
        return *this;
    }
};

struct StandardMesh {
    std::vector<StandardVertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<TextureData> textures;
    std::vector<float> vertexBuffer;
};

class ModelParser {
private:
    std::vector<StandardMesh> meshes;
    std::string directory;
    
    void processNode(void* node, void* scene);
    StandardMesh processMesh(void* mesh, void* scene);
    void createVertexBuffer(StandardMesh& mesh);
    std::vector<TextureData> loadMaterialTextures(void* mat, unsigned int texType, std::string typeName, void* scene);
    RawTextureData textureFromEmbedded(const void* embeddedTexture);
    RawTextureData textureFromFile(const char* path, const std::string& directory);
    
public:
    ModelParser();
    ~ModelParser();
    bool loadModel(const std::string& path);
    const std::vector<StandardMesh>& getMeshes() const { return meshes; }
    void printVertexInfo();
    void cleanupTextures();
};

#endif