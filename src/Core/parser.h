#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <string>
#include <assimp/scene.h>

struct StandardVertex {
    float position[3];
    float normal[3];
    float texCoords[2];
};

struct StandardMesh {
    std::vector<StandardVertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<float> vertexBuffer;
};

class ModelParser {
public:
    ModelParser();
    bool loadModel(const std::string& path);
    const std::vector<StandardMesh>& getMeshes() const { return meshes; }
    void printVertexInfo();
    
private:
    void processNode(aiNode* node, const aiScene* scene);
    StandardMesh processMesh(aiMesh* mesh, const aiScene* scene);
    void createVertexBuffer(StandardMesh& mesh);
    
    std::vector<StandardMesh> meshes;
    std::string directory;
};

#endif