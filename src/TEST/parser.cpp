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
        std::cout << "Warning: No directory found in path, using current dir" << std::endl;
        directory = ".";
    }
    
    // ПРОВЕРКА НАЛИЧИЯ МАТЕРИАЛОВ И ТЕКСТУР
    std::cout << "\n=== CHECKING MATERIALS AND TEXTURES ===" << std::endl;
    std::cout << "Total materials in scene: " << scene->mNumMaterials << std::endl;
    
    for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
        aiMaterial* material = scene->mMaterials[i];
        if (material) {
            aiString matName;
            material->Get(AI_MATKEY_NAME, matName);
            std::cout << "Material " << i << ": " << matName.C_Str() << std::endl;
            
            // Проверка диффузных текстур
            int texCount = material->GetTextureCount(aiTextureType_DIFFUSE);
            std::cout << "  Diffuse textures count: " << texCount << std::endl;
            for (int t = 0; t < texCount; t++) {
                aiString path;
                material->GetTexture(aiTextureType_DIFFUSE, t, &path);
                std::cout << "    Texture " << t << ": " << path.C_Str() << std::endl;
                
                // Проверка существования файла текстуры
                std::string texPath = path.C_Str();
                if (std::filesystem::exists(texPath)) {
                    std::cout << "      [OK] File exists: " << texPath << std::endl;
                } else {
                    // Проверка относительно директории модели
                    std::string fullPath = directory + "/" + texPath;
                    if (std::filesystem::exists(fullPath)) {
                        std::cout << "      [OK] File exists (relative): " << fullPath << std::endl;
                    } else {
                        std::cout << "      [WARNING] Texture file NOT FOUND: " << texPath << std::endl;
                    }
                }
            }
            
            // Проверка встроенных текстур
            int embeddedCount = scene->mNumTextures;
            if (embeddedCount > 0) {
                std::cout << "  Embedded textures in scene: " << embeddedCount << std::endl;
                for (unsigned int e = 0; e < embeddedCount; e++) {
                    if (scene->mTextures[e]) {
                        std::cout << "    Embedded texture " << e << ": " 
                                  << scene->mTextures[e]->mFilename.C_Str() 
                                  << " (size: " << scene->mTextures[e]->mWidth << ")" << std::endl;
                    }
                }
            }
        }
    }
    std::cout << "=======================================\n" << std::endl;
    
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
    
    if (!ai_node || !ai_scene) {
        std::cout << "ERROR: Null node or scene in processNode!" << std::endl;
        return;
    }
    
    for(unsigned int i = 0; i < ai_node->mNumMeshes; i++) {
        if (ai_node->mMeshes[i] >= ai_scene->mNumMeshes) {
            std::cout << "ERROR: Mesh index out of bounds!" << std::endl;
            continue;
        }
        
        aiMesh* mesh = ai_scene->mMeshes[ai_node->mMeshes[i]];
        if (!mesh) {
            std::cout << "ERROR: Null mesh pointer!" << std::endl;
            continue;
        }
        
        meshes.push_back(processMesh(mesh, (void*)ai_scene));
    }
    
    for(unsigned int i = 0; i < ai_node->mNumChildren; i++) {
        if (ai_node->mChildren[i]) {
            processNode(ai_node->mChildren[i], (void*)ai_scene);
        }
    }
}

StandardMesh ModelParser::processMesh(void* mesh_ptr, void* scene_ptr) {
    aiMesh* mesh = (aiMesh*)mesh_ptr;
    const aiScene* scene = (const aiScene*)scene_ptr;
    
    StandardMesh standardMesh;
    
    if (!mesh) {
        std::cout << "ERROR: processMesh called with null mesh!" << std::endl;
        return standardMesh;
    }
    
    std::cout << "Processing mesh: " << mesh->mName.C_Str() 
              << " (vertices: " << mesh->mNumVertices 
              << ", faces: " << mesh->mNumFaces 
              << ", material: " << mesh->mMaterialIndex << ")" << std::endl;
    
    // Вершины
    for(unsigned int i = 0; i < mesh->mNumVertices; i++) {
        StandardVertex vertex;
        
        if (mesh->mVertices) {
            vertex.position[0] = mesh->mVertices[i].x;
            vertex.position[1] = mesh->mVertices[i].y;
            vertex.position[2] = mesh->mVertices[i].z;
        } else {
            vertex.position[0] = vertex.position[1] = vertex.position[2] = 0.0f;
        }
        
        if (mesh->HasNormals() && mesh->mNormals) {
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
    
    // Индексы
    if (mesh->mFaces) {
        for(unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            if (face.mIndices) {
                for(unsigned int j = 0; j < face.mNumIndices; j++) {
                    standardMesh.indices.push_back(face.mIndices[j]);
                }
            }
        }
    }
    
    // ЗАГРУЗКА ТЕКСТУР ДЛЯ VULKAN (ВСЕГДА, БЕЗ ФЛАГА)
    // Сохраняем пути к текстурам, даже если OpenGL выключен
    if(mesh->mMaterialIndex >= 0 && mesh->mMaterialIndex < (int)scene->mNumMaterials) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        
        // Загружаем диффузные текстуры
        for(unsigned int t = 0; t < material->GetTextureCount(aiTextureType_DIFFUSE); t++) {
            aiString texPath;
            material->GetTexture(aiTextureType_DIFFUSE, t, &texPath);
            
            Texture tex;
            tex.type = "texture_diffuse";
            tex.path = texPath.C_Str();
            
            // Проверяем встроенную текстуру
            bool isEmbedded = false;
            for(unsigned int e = 0; e < scene->mNumTextures; e++) {
                if(scene->mTextures[e] && scene->mTextures[e]->mFilename.C_Str() == texPath.C_Str()) {
                    tex.id = e + 1000; // Маркер встроенной текстуры
                    isEmbedded = true;
                    std::cout << "  Found embedded texture: " << texPath.C_Str() << std::endl;
                    break;
                }
            }
            
            if(!isEmbedded) {
                // Проверяем существование файла
                std::string fullPath = directory + "/" + tex.path;
                if(std::filesystem::exists(tex.path) || std::filesystem::exists(fullPath)) {
                    std::cout << "  Found external texture: " << tex.path << std::endl;
                    tex.id = 0; // ID будет установлен при загрузке в Vulkan
                } else {
                    std::cout << "  [WARNING] Texture file missing: " << tex.path << std::endl;
                    tex.id = 0;
                }
            }
            
            standardMesh.textures.push_back(tex);
            textures_loaded.push_back(tex);
        }
    }
    
    createVertexBuffer(standardMesh);
    
    std::cout << "Mesh processed: " << standardMesh.vertices.size() 
              << " vertices, " << standardMesh.indices.size() << " indices, "
              << standardMesh.textures.size() << " textures" << std::endl;
    
    return standardMesh;
}

void ModelParser::createVertexBuffer(StandardMesh& mesh) {
    mesh.vertexBuffer.clear();
    
    if (mesh.vertices.empty()) {
        std::cout << "ERROR: createVertexBuffer called with empty vertices!" << std::endl;
        return;
    }
    
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
    std::cout << "\n=== STANDARDIZED VERTEX INFORMATION ===" << std::endl;
    std::cout << "Total meshes: " << meshes.size() << std::endl;
    
    if (meshes.empty()) {
        std::cout << "ERROR: No meshes to display!" << std::endl;
        return;
    }
    
    size_t totalVertices = 0;
    size_t totalIndices = 0;
    size_t totalBufferFloats = 0;
    size_t totalTextures = 0;
    
    for (size_t meshIndex = 0; meshIndex < meshes.size(); meshIndex++) {
        const StandardMesh& mesh = meshes[meshIndex];
        std::cout << "\n--- Mesh " << meshIndex << " ---" << std::endl;
        std::cout << "Vertices: " << mesh.vertices.size() << std::endl;
        std::cout << "Indices: " << mesh.indices.size() << std::endl;
        std::cout << "Textures: " << mesh.textures.size() << std::endl;
        
        totalVertices += mesh.vertices.size();
        totalIndices += mesh.indices.size();
        totalBufferFloats += mesh.vertexBuffer.size();
        totalTextures += mesh.textures.size();
        
        if (!mesh.textures.empty()) {
            std::cout << "Texture list:" << std::endl;
            for (const auto& tex : mesh.textures) {
                std::cout << "  - Type: " << tex.type << ", Path: " << tex.path << std::endl;
            }
        }
        
        if (mesh.vertices.empty()) {
            std::cout << "ERROR: Mesh has no vertices!" << std::endl;
            continue;
        }
        
        if (meshIndex == 0) {
            const StandardVertex& vertex = mesh.vertices[0];
            std::cout << "First vertex sample: ";
            std::cout << "Pos(" << vertex.position[0] << ", " 
                      << vertex.position[1] << ", " << vertex.position[2] << ") ";
            std::cout << "UV(" << vertex.texCoords[0] << ", " << vertex.texCoords[1] << ")" << std::endl;
        }
    }
    
    std::cout << "\n=== SUMMARY ===" << std::endl;
    std::cout << "Total vertices: " << totalVertices << std::endl;
    std::cout << "Total indices: " << totalIndices << std::endl;
    std::cout << "Total textures: " << totalTextures << std::endl;
    std::cout << "=== END VERTEX INFORMATION ===\n" << std::endl;
}