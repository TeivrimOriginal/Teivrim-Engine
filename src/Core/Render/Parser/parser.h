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

struct Texture {
    unsigned int id;
    std::string type;
    std::string path;
};

struct StandardMesh {
    std::vector<StandardVertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;
    std::vector<float> vertexBuffer;
};

class ModelParser {
private:
    std::vector<StandardMesh> meshes;
    std::string directory;
    std::vector<Texture> textures_loaded;
    
    void processNode(void* node, void* scene);
    StandardMesh processMesh(void* mesh, void* scene);
    void createVertexBuffer(StandardMesh& mesh);
    std::vector<Texture> loadMaterialTextures(void* mat, unsigned int texType, std::string typeName, void* scene);
    unsigned int textureFromFile(const char* path, const std::string& directory);
    unsigned int textureFromEmbedded(const void* embeddedTexture);
    
public:
    ModelParser();
    bool loadModel(const std::string& path);
    const std::vector<StandardMesh>& getMeshes() const { return meshes; }
    void printVertexInfo();
};

#endif