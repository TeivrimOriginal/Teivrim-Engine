#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "parser.h"
#include <iostream>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <GL/glew.h>

#include <cstring>

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
        
        // Позиция
        if (mesh->mVertices) {
            vertex.position[0] = mesh->mVertices[i].x;
            vertex.position[1] = mesh->mVertices[i].y;
            vertex.position[2] = mesh->mVertices[i].z;
        } else {
            std::cout << "ERROR: No vertex positions in mesh!" << std::endl;
            vertex.position[0] = vertex.position[1] = vertex.position[2] = 0.0f;
        }
        
        // Нормали
        if (mesh->HasNormals() && mesh->mNormals) {
            vertex.normal[0] = mesh->mNormals[i].x;
            vertex.normal[1] = mesh->mNormals[i].y;
            vertex.normal[2] = mesh->mNormals[i].z;
        } else {
            vertex.normal[0] = 0.0f;
            vertex.normal[1] = 1.0f;
            vertex.normal[2] = 0.0f;
        }
        
        // Текстурные координаты
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
            } else {
                std::cout << "ERROR: Face has null indices!" << std::endl;
            }
        }
    } else {
        std::cout << "ERROR: Mesh has no faces!" << std::endl;
    }
    
    // Материалы и текстуры
    if(mesh->mMaterialIndex >= 0) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        
        // Диффузные текстуры
        std::vector<Texture> diffuseMaps = loadMaterialTextures(
            material, aiTextureType_DIFFUSE, "texture_diffuse", (void*)scene);
        standardMesh.textures.insert(standardMesh.textures.end(), 
            diffuseMaps.begin(), diffuseMaps.end());
        
        // Спекулярные текстуры
        std::vector<Texture> specularMaps = loadMaterialTextures(
            material, aiTextureType_SPECULAR, "texture_specular", (void*)scene);
        standardMesh.textures.insert(standardMesh.textures.end(), 
            specularMaps.begin(), specularMaps.end());
    }
    
    createVertexBuffer(standardMesh);
    
    std::cout << "Mesh processed: " << standardMesh.vertices.size() 
              << " vertices, " << standardMesh.indices.size() << " indices, "
              << standardMesh.textures.size() << " textures" << std::endl;
    
    return standardMesh;
}

std::vector<Texture> ModelParser::loadMaterialTextures(void* mat_ptr, 
    unsigned int texType, std::string typeName, void* scene_ptr) {
    
    aiMaterial* mat = (aiMaterial*)mat_ptr;
    const aiScene* scene = (const aiScene*)scene_ptr;
    std::vector<Texture> textures;
    
    for(unsigned int i = 0; i < mat->GetTextureCount((aiTextureType)texType); i++) {
        aiString str;
        mat->GetTexture((aiTextureType)texType, i, &str);
        
        // Проверяем, не загружали ли уже эту текстуру
        bool skip = false;
        for(unsigned int j = 0; j < textures_loaded.size(); j++) {
            if(std::strcmp(textures_loaded[j].path.c_str(), str.C_Str()) == 0) {
                textures.push_back(textures_loaded[j]);
                skip = true;
                break;
            }
        }
        
        if(!skip) {
            // Ищем встроенную текстуру
            const aiTexture* embeddedTexture = nullptr;
            
            // Проверяем все встроенные текстуры в сцене
            for(unsigned int j = 0; j < scene->mNumTextures; j++) {
                if(scene->mTextures[j]->mFilename.C_Str() == std::string(str.C_Str())) {
                    embeddedTexture = scene->mTextures[j];
                    break;
                }
            }
            
            Texture texture;
            if(embeddedTexture) {
                // ВСТРОЕННАЯ ТЕКСТУРА
                std::cout << "Загружаем встроенную текстуру: " << str.C_Str() << std::endl;
                texture.id = textureFromEmbedded(embeddedTexture);
                if(texture.id != 0) {
                    texture.type = typeName;
                    texture.path = str.C_Str();
                    textures.push_back(texture);
                    textures_loaded.push_back(texture);
                }
            } else {
                // ВНЕШНЯЯ ТЕКСТУРА
                std::cout << "Загружаем внешнюю текстуру: " << str.C_Str() << std::endl;
                texture.id = textureFromFile(str.C_Str(), directory);
                if(texture.id != 0) {
                    texture.type = typeName;
                    texture.path = str.C_Str();
                    textures.push_back(texture);
                    textures_loaded.push_back(texture);
                }
            }
        }
    }
    return textures;
}

unsigned int ModelParser::textureFromEmbedded(const void* embeddedTexture_ptr) {
    const aiTexture* embeddedTexture = (const aiTexture*)embeddedTexture_ptr;
    unsigned int textureID;
    glGenTextures(1, &textureID);
    
    int width, height, nrComponents;
    unsigned char* data = nullptr;
    
    if(embeddedTexture->mHeight == 0) {
        // Сжатый формат (JPEG, PNG и т.д.)
        std::cout << "Текстура сжатая, размер: " << embeddedTexture->mWidth << " байт" << std::endl;
        data = stbi_load_from_memory(
            reinterpret_cast<unsigned char*>(embeddedTexture->pcData),
            embeddedTexture->mWidth,
            &width,
            &height,
            &nrComponents,
            0
        );
    } else {
        // Несжатый ARGB формат
        std::cout << "Текстура несжатая, размер: " 
                  << embeddedTexture->mWidth << "x" << embeddedTexture->mHeight << std::endl;
        // Конвертируем из ARGB в RGBA
        data = new unsigned char[embeddedTexture->mWidth * embeddedTexture->mHeight * 4];
        for(unsigned int i = 0; i < embeddedTexture->mWidth * embeddedTexture->mHeight; ++i) {
            data[i*4] = embeddedTexture->pcData[i].b;
            data[i*4+1] = embeddedTexture->pcData[i].g;
            data[i*4+2] = embeddedTexture->pcData[i].r;
            data[i*4+3] = embeddedTexture->pcData[i].a;
        }
        width = embeddedTexture->mWidth;
        height = embeddedTexture->mHeight;
        nrComponents = 4;
    }
    
    if(data) {
        GLenum format;
        if(nrComponents == 1)
            format = GL_RED;
        else if(nrComponents == 3)
            format = GL_RGB;
        else if(nrComponents == 4)
            format = GL_RGBA;
        
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        std::cout << "Текстура загружена: " << width << "x" << height 
                  << ", каналов: " << nrComponents << std::endl;
        
        if(embeddedTexture->mHeight == 0) {
            stbi_image_free(data);
        } else {
            delete[] data;
        }
    } else {
        std::cout << "Не удалось загрузить встроенную текстуру" << std::endl;
        glDeleteTextures(1, &textureID);
        return 0;
    }
    
    return textureID;
}

unsigned int ModelParser::textureFromFile(const char* path, const std::string& directory) {
    std::string filename = std::string(path);
    
    unsigned char* data = nullptr;
    int width, height, nrComponents;
    
    // 1. Пробуем как абсолютный путь
    data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 4); // ПРИНУДИТЕЛЬНО 4 КАНАЛА
    
    if(!data && !directory.empty()) {
        // 2. Пробуем относительно директории модели
        filename = directory + '/' + std::string(path);
        data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 4); // ПРИНУДИТЕЛЬНО 4 КАНАЛА
    }
    
    if(!data) {
        // 3. Пробуем только имя файла
        std::string simpleName = std::string(path);
        size_t pos = simpleName.find_last_of("/\\");
        if(pos != std::string::npos) {
            simpleName = simpleName.substr(pos + 1);
        }
        data = stbi_load(simpleName.c_str(), &width, &height, &nrComponents, 4); // ПРИНУДИТЕЛЬНО 4 КАНАЛА
    }
    
    if(!data) {
        std::cout << "Текстура не загружена: " << path << std::endl;
        return 0;
    }
    
    unsigned int textureID;
    glGenTextures(1, &textureID);
    
    // ВСЕГДА ИСПОЛЬЗУЕМ GL_RGBA
    GLenum format = GL_RGBA;
    
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    stbi_image_free(data);
    
    std::cout << "Текстура загружена: " << width << "x" << height << ", каналов: 4 (RGBA)" << std::endl;
    
    return textureID;
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
        
        if (meshIndex == 0) { // Показываем только для первого меша
            std::cout << "First vertex sample: ";
            const StandardVertex& vertex = mesh.vertices[0];
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