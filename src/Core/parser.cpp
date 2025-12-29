#include "parser.h"
#include <iostream>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

ModelParser::ModelParser() {}

bool ModelParser::loadModel(const std::string& path) {
    Assimp::Importer import;
    const aiScene* scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);
    
    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
        return false;
    }
    
    directory = path.substr(0, path.find_last_of('/'));
    processNode(scene->mRootNode, scene);
    
    printVertexInfo();
    return true;
}

void ModelParser::processNode(aiNode* node, const aiScene* scene) {
    for(unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]]; 
        meshes.push_back(processMesh(mesh, scene));			
    }
    
    for(unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene);
    }
}

StandardMesh ModelParser::processMesh(aiMesh* mesh, const aiScene* scene) {
    StandardMesh standardMesh;
    
    for(unsigned int i = 0; i < mesh->mNumVertices; i++) {
        StandardVertex vertex;
        
        vertex.position[0] = mesh->mVertices[i].x;
        vertex.position[1] = mesh->mVertices[i].y;
        vertex.position[2] = mesh->mVertices[i].z;
        
        if (mesh->HasNormals()) {
            vertex.normal[0] = mesh->mNormals[i].x;
            vertex.normal[1] = mesh->mNormals[i].y;
            vertex.normal[2] = mesh->mNormals[i].z;
        } else {
            vertex.normal[0] = 0.0f;
            vertex.normal[1] = 1.0f;
            vertex.normal[2] = 0.0f;
        }
        
        if(mesh->mTextureCoords[0]) {
            vertex.texCoords[0] = mesh->mTextureCoords[0][i].x; 
            vertex.texCoords[1] = mesh->mTextureCoords[0][i].y;
        } else {
            vertex.texCoords[0] = 0.0f;
            vertex.texCoords[1] = 0.0f;
        }
        
        standardMesh.vertices.push_back(vertex);
    }
    
    for(unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for(unsigned int j = 0; j < face.mNumIndices; j++)
            standardMesh.indices.push_back(face.mIndices[j]);        
    }
    
    createVertexBuffer(standardMesh);
    
    return standardMesh;
}

void ModelParser::createVertexBuffer(StandardMesh& mesh) {
    mesh.vertexBuffer.clear();
    
    for (const auto& vertex : mesh.vertices) {
        mesh.vertexBuffer.push_back(vertex.position[0]);
        mesh.vertexBuffer.push_back(vertex.position[1]);
        mesh.vertexBuffer.push_back(vertex.position[2]);
        
        mesh.vertexBuffer.push_back(vertex.normal[0]);
        mesh.vertexBuffer.push_back(vertex.normal[1]);
        mesh.vertexBuffer.push_back(vertex.normal[2]);
        
        mesh.vertexBuffer.push_back(vertex.texCoords[0]);
        mesh.vertexBuffer.push_back(vertex.texCoords[1]);
    }
}

void ModelParser::printVertexInfo() {
    std::cout << "\n=== STANDARDIZED VERTEX INFORMATION ===" << std::endl;
    std::cout << "Total meshes: " << meshes.size() << std::endl;
    
    for (size_t meshIndex = 0; meshIndex < meshes.size(); meshIndex++) {
        const StandardMesh& mesh = meshes[meshIndex];
        std::cout << "\n--- Mesh " << meshIndex << " ---" << std::endl;
        std::cout << "Vertices: " << mesh.vertices.size() << std::endl;
        std::cout << "Indices: " << mesh.indices.size() << std::endl;
        std::cout << "Vertex buffer size: " << mesh.vertexBuffer.size() << " floats" << std::endl;
        
        std::cout << "First 5 vertices:" << std::endl;
        for (size_t i = 0; i < std::min((size_t)5, mesh.vertices.size()); i++) {
            const StandardVertex& vertex = mesh.vertices[i];
            std::cout << "  V" << i << ": Pos(" 
                      << vertex.position[0] << ", " << vertex.position[1] << ", " << vertex.position[2] << ")" << std::endl;
        }
    }
    std::cout << "=== END VERTEX INFORMATION ===\n" << std::endl;
}