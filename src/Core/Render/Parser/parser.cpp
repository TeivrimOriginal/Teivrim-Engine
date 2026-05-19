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

ModelParser::~ModelParser() {
    cleanupTextures();
}

void ModelParser::cleanupTextures() {
    for (auto& mesh : meshes) {
        for (auto& tex : mesh.textures) {
            if (tex.rawData.data) {
                delete[] tex.rawData.data;
                tex.rawData.data = nullptr;
            }
        }
    }
}

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
    
    std::cout << "Processing node: " << ai_node->mName.C_Str() 
              << " (meshes: " << ai_node->mNumMeshes 
              << ", children: " << ai_node->mNumChildren << ")" << std::endl;
    
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
        std::cout << "  Added mesh " << i << " with " << mesh->mNumVertices << " vertices" << std::endl;
    }
    
    for(unsigned int i = 0; i < ai_node->mNumChildren; i++) {
        if (ai_node->mChildren[i]) {
            processNode(ai_node->mChildren[i], (void*)ai_scene);
        } else {
            std::cout << "ERROR: Null child node!" << std::endl;
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
              << ", faces: " << mesh->mNumFaces << ")" << std::endl;
    
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
        
// В processMesh:
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
    
    // Текстуры
    if(mesh->mMaterialIndex >= 0) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        
        std::vector<TextureData> diffuseMaps = loadMaterialTextures(
            material, aiTextureType_DIFFUSE, "texture_diffuse", (void*)scene);
        
        for (auto& tex : diffuseMaps) {
            standardMesh.textures.push_back(std::move(tex));
        }
        
        std::vector<TextureData> specularMaps = loadMaterialTextures(
            material, aiTextureType_SPECULAR, "texture_specular", (void*)scene);
        
        for (auto& tex : specularMaps) {
            standardMesh.textures.push_back(std::move(tex));
        }
    }
    
    createVertexBuffer(standardMesh);
    
    std::cout << "Mesh processed: " << standardMesh.vertices.size() 
              << " vertices, " << standardMesh.indices.size() << " indices, "
              << standardMesh.textures.size() << " textures" << std::endl;
    
    return standardMesh;
}

std::vector<TextureData> ModelParser::loadMaterialTextures(void* mat_ptr, 
    unsigned int texType, std::string typeName, void* scene_ptr) {
    
    aiMaterial* mat = (aiMaterial*)mat_ptr;
    const aiScene* scene = (const aiScene*)scene_ptr;
    std::vector<TextureData> textures;
    
    for(unsigned int i = 0; i < mat->GetTextureCount((aiTextureType)texType); i++) {
        aiString str;
        mat->GetTexture((aiTextureType)texType, i, &str);
        
        // Проверяем, не загружали ли уже эту текстуру
        bool skip = false;
        for(auto& existingTex : textures) {
            if(existingTex.rawData.path == str.C_Str()) {
                skip = true;
                break;
            }
        }
        
        if(!skip) {
            // Ищем встроенную текстуру
            const aiTexture* embeddedTexture = nullptr;
            for(unsigned int j = 0; j < scene->mNumTextures; j++) {
                if(scene->mTextures[j]->mFilename.C_Str() == std::string(str.C_Str())) {
                    embeddedTexture = scene->mTextures[j];
                    break;
                }
            }
            
            TextureData textureData;
            textureData.type = typeName;
            
            if(embeddedTexture) {
                std::cout << "Loading embedded texture: " << str.C_Str() << std::endl;
                textureData.rawData = textureFromEmbedded(embeddedTexture);
            } else {
                std::cout << "Loading external texture: " << str.C_Str() << std::endl;
                textureData.rawData = textureFromFile(str.C_Str(), directory);
            }
            
            if(textureData.rawData.isValid) {
                textureData.rawData.path = str.C_Str();
                textures.push_back(std::move(textureData));
            }
        }
    }
    return textures;
}

RawTextureData ModelParser::textureFromEmbedded(const void* embeddedTexture_ptr) {
    const aiTexture* embeddedTexture = (const aiTexture*)embeddedTexture_ptr;
    RawTextureData result;
    
    int width, height, channels;
    unsigned char* data = nullptr;
    
    if(embeddedTexture->mHeight == 0) {
        // Сжатый формат (JPEG, PNG)
        std::cout << "Compressed embedded texture, size: " << embeddedTexture->mWidth << " bytes" << std::endl;
        data = stbi_load_from_memory(
            reinterpret_cast<const unsigned char*>(embeddedTexture->pcData),
            embeddedTexture->mWidth,
            &width,
            &height,
            &channels,
            4  // force RGBA
        );
    } else {
        // Несжатый ARGB формат
        std::cout << "Uncompressed embedded texture: " 
                  << embeddedTexture->mWidth << "x" << embeddedTexture->mHeight << std::endl;
        width = embeddedTexture->mWidth;
        height = embeddedTexture->mHeight;
        channels = 4;
        data = new unsigned char[width * height * 4];
        for(unsigned int i = 0; i < width * height; ++i) {
            data[i*4] = embeddedTexture->pcData[i].b;
            data[i*4+1] = embeddedTexture->pcData[i].g;
            data[i*4+2] = embeddedTexture->pcData[i].r;
            data[i*4+3] = embeddedTexture->pcData[i].a;
        }
    }
    
    if(data) {
        result.data = data;
        result.width = width;
        result.height = height;
        result.channels = 4;
        result.isValid = true;
        std::cout << "Texture loaded: " << width << "x" << height << ", channels: 4" << std::endl;
    } else {
        std::cout << "Failed to load embedded texture" << std::endl;
    }
    
    return result;
}

RawTextureData ModelParser::textureFromFile(const char* path, const std::string& directory) {
    std::string filename = std::string(path);
    RawTextureData result;
    
    int width, height, channels;
    unsigned char* data = nullptr;
    
    // 1. Пробуем как абсолютный путь
    data = stbi_load(filename.c_str(), &width, &height, &channels, 4);
    
    if(!data && !directory.empty()) {
        // 2. Пробуем относительно директории модели
        filename = directory + '/' + std::string(path);
        data = stbi_load(filename.c_str(), &width, &height, &channels, 4);
    }
    
    if(!data) {
        // 3. Пробуем только имя файла
        std::string simpleName = std::string(path);
        size_t pos = simpleName.find_last_of("/\\");
        if(pos != std::string::npos) {
            simpleName = simpleName.substr(pos + 1);
        }
        data = stbi_load(simpleName.c_str(), &width, &height, &channels, 4);
    }
    
    if(data) {
        result.data = data;
        result.width = width;
        result.height = height;
        result.channels = 4;
        result.isValid = true;
        std::cout << "Texture loaded from file: " << width << "x" << height << std::endl;
    } else {
        std::cout << "Failed to load texture: " << path << std::endl;
    }
    
    return result;
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
    
    std::cout << "Vertex buffer created: " << mesh.vertexBuffer.size() 
              << " floats (" << mesh.vertices.size() << " vertices)" << std::endl;
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
        std::cout << "Vertex buffer size: " << mesh.vertexBuffer.size() << " floats" << std::endl;
        
        totalVertices += mesh.vertices.size();
        totalIndices += mesh.indices.size();
        totalBufferFloats += mesh.vertexBuffer.size();
        totalTextures += mesh.textures.size();
        
        if (!mesh.textures.empty()) {
            std::cout << "Texture types: ";
            for (const auto& tex : mesh.textures) {
                std::cout << tex.type << " ";
            }
            std::cout << std::endl;
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
            std::cout << "Norm(" << vertex.normal[0] << ", " 
                      << vertex.normal[1] << ", " << vertex.normal[2] << ") ";
            std::cout << "UV(" << vertex.texCoords[0] << ", " << vertex.texCoords[1] << ")" << std::endl;
        }
    }
    
    std::cout << "\n=== SUMMARY ===" << std::endl;
    std::cout << "Total vertices: " << totalVertices << std::endl;
    std::cout << "Total indices: " << totalIndices << std::endl;
    std::cout << "Total textures: " << totalTextures << std::endl;
    std::cout << "Total buffer floats: " << totalBufferFloats << std::endl;
    std::cout << "Memory usage: ~" << (totalBufferFloats * sizeof(float) / 1024.0f) << " KB" << std::endl;
    std::cout << "=== END VERTEX INFORMATION ===\n" << std::endl;
}