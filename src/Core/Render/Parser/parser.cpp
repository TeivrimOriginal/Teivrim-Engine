#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "parser.h"
#include <iostream>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <cstring>
#include <filesystem>

ModelParser::ModelParser() {}

bool ModelParser::loadModel(const std::string& path) {
    Assimp::Importer import;
    const aiScene* scene = import.ReadFile(path.c_str(), 
        aiProcess_Triangulate | 
        aiProcess_FlipUVs | 
        aiProcess_GenSmoothNormals |
        aiProcess_CalcTangentSpace);
    
    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
        std::cout << "Failed to load model: " << path << std::endl;
        return false;
    }

    std::cout << "Assimp: Model loaded successfully: " << path << std::endl;
    
    directory = path.substr(0, path.find_last_of('/'));
    if (directory == path) {
        directory = path.substr(0, path.find_last_of('\\'));
    }
    if (directory == path) {
        directory = ".";
    }
    
    std::cout << "Processing model structure..." << std::endl;
    processNode(scene->mRootNode, (void*)scene);
    
    if (meshes.empty()) {
        std::cout << "ERROR: No meshes found in the model!" << std::endl;
        return false;
    }
    
    std::cout << "Model parsed successfully" << std::endl;
    printVertexInfo();
    return true;
}

void ModelParser::processNode(void* node, void* scene) {
    aiNode* ai_node = (aiNode*)node;
    const aiScene* ai_scene = (const aiScene*)scene;
    
    if (!ai_node || !ai_scene) return;
    
    for(unsigned int i = 0; i < ai_node->mNumMeshes; i++) {
        if (ai_node->mMeshes[i] >= ai_scene->mNumMeshes) continue;
        
        aiMesh* mesh = ai_scene->mMeshes[ai_node->mMeshes[i]];
        if (!mesh) continue;
        
        meshes.push_back(processMesh(mesh, (void*)ai_scene));
    }
    
    for(unsigned int i = 0; i < ai_node->mNumChildren; i++) {
        if (ai_node->mChildren[i]) processNode(ai_node->mChildren[i], (void*)ai_scene);
    }
}

StandardMesh ModelParser::processMesh(void* mesh_ptr, void* scene_ptr) {
    aiMesh* mesh = (aiMesh*)mesh_ptr;
    const aiScene* scene = (const aiScene*)scene_ptr;
    
    StandardMesh standardMesh;
    
    if (!mesh) return standardMesh;
    
    // Вершины
    for(unsigned int i = 0; i < mesh->mNumVertices; i++) {
        StandardVertex vertex;
        
        vertex.position[0] = mesh->mVertices[i].x;
        vertex.position[1] = mesh->mVertices[i].y;
        vertex.position[2] = mesh->mVertices[i].z;
        
        if (mesh->HasNormals() && mesh->mNormals) {
            vertex.normal[0] = mesh->mNormals[i].x;
            vertex.normal[1] = mesh->mNormals[i].y;
            vertex.normal[2] = mesh->mNormals[i].z;
        } else {
            vertex.normal[0] = 0.0f; vertex.normal[1] = 1.0f; vertex.normal[2] = 0.0f;
        }
        
        if(mesh->mTextureCoords[0]) {
            vertex.texCoords[0] = mesh->mTextureCoords[0][i].x; 
            vertex.texCoords[1] = mesh->mTextureCoords[0][i].y;
        } else {
            vertex.texCoords[0] = 0.0f; vertex.texCoords[1] = 0.0f;
        }
                
        standardMesh.vertices.push_back(vertex);
    }
    
    // Индексы
    for(unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for(unsigned int j = 0; j < face.mNumIndices; j++) {
            standardMesh.indices.push_back(face.mIndices[j]);
        }
    }
    
    // Сохраняем пути к текстурам (без загрузки через OpenGL)
    if(mesh->mMaterialIndex >= 0 && mesh->mMaterialIndex < (int)scene->mNumMaterials) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        
        for(unsigned int t = 0; t < material->GetTextureCount(aiTextureType_DIFFUSE); t++) {
            aiString texPath;
            material->GetTexture(aiTextureType_DIFFUSE, t, &texPath);
            
            Texture tex;
            tex.type = "texture_diffuse";
            tex.path = texPath.C_Str();
            tex.id = 0;
            
            // Проверяем встроенная ли текстура
            for(unsigned int e = 0; e < scene->mNumTextures; e++) {
                if(scene->mTextures[e] && scene->mTextures[e]->mFilename.C_Str() == texPath.C_Str()) {
                    tex.id = e + 1000;
                    break;
                }
            }
            
            standardMesh.textures.push_back(tex);
        }
    }
    
    createVertexBuffer(standardMesh);
    
    return standardMesh;
}

void ModelParser::createVertexBuffer(StandardMesh& mesh) {
    mesh.vertexBuffer.clear();
    
    if (mesh.vertices.empty()) return;
    
    for (size_t i = 0; i < mesh.vertices.size(); i++) {
        const StandardVertex& vertex = mesh.vertices[i];
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
    std::cout << "\n=== MODEL INFO ===" << std::endl;
    std::cout << "Total meshes: " << meshes.size() << std::endl;
    
    size_t totalVertices = 0, totalIndices = 0, totalTextures = 0;
    
    for (size_t i = 0; i < meshes.size(); i++) {
        const StandardMesh& mesh = meshes[i];
        std::cout << "Mesh " << i << ": " << mesh.vertices.size() 
                  << " vertices, " << mesh.indices.size() 
                  << " indices, " << mesh.textures.size() << " textures" << std::endl;
        
        totalVertices += mesh.vertices.size();
        totalIndices += mesh.indices.size();
        totalTextures += mesh.textures.size();
        
        if (!mesh.textures.empty() && i == 0) {
            std::cout << "  Textures: ";
            for (const auto& tex : mesh.textures) std::cout << tex.path << " ";
            std::cout << std::endl;
        }
    }
    
    std::cout << "Total: " << totalVertices << " vertices, " 
              << totalIndices << " indices, " << totalTextures << " textures" << std::endl;
    std::cout << "================\n" << std::endl;
}