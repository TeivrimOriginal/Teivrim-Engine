// SceneManager.h - ПОЛНАЯ ЗАМЕНА
#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "../../Render/Parser/parser.h"

class Vulkan;

struct Transform {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);  // Euler angles in degrees
    glm::vec3 scale = glm::vec3(1.0f);
    
    glm::mat4 GetMatrix() const {
        glm::mat4 mat = glm::translate(glm::mat4(1.0f), position);
        mat = glm::rotate(mat, glm::radians(rotation.x), glm::vec3(1, 0, 0));
        mat = glm::rotate(mat, glm::radians(rotation.y), glm::vec3(0, 1, 0));
        mat = glm::rotate(mat, glm::radians(rotation.z), glm::vec3(0, 0, 1));
        mat = glm::scale(mat, scale);
        return mat;
    }
};

struct SceneObject {
    std::string name;
    std::string modelPath;
    ModelParser* parser;
    Transform localTransform;
    Transform worldTransform;  // Calculated from parent
    SceneObject* parent;
    std::vector<SceneObject*> children;
    bool visible;
    bool loaded;
    int meshCount;  // Number of meshes in this object
    
    SceneObject() : parser(nullptr), parent(nullptr), visible(true), loaded(false), meshCount(0) {}
    ~SceneObject() { delete parser; }
};

class SceneManager {
public:
    static SceneManager& Instance() {
        static SceneManager instance;
        return instance;
    }
    
    // ===== MAIN METHODS =====
    
    // Create empty object at root level
    SceneObject* CreateEmpty(const std::string& name) {
        SceneObject* obj = new SceneObject();
        obj->name = name;
        obj->parent = nullptr;
        rootObjects.push_back(obj);
        allObjects.push_back(obj);
        return obj;
    }
    
    // Load model from path and create object
    SceneObject* LoadModel(const std::string& name, const std::string& path) {
        SceneObject* obj = new SceneObject();
        obj->name = name;
        obj->modelPath = path;
        obj->parser = new ModelParser();
        obj->parent = nullptr;
        
        if (obj->parser->loadModel(path)) {
            obj->loaded = true;
            obj->meshCount = (int)obj->parser->getMeshes().size();
            std::cout << "[SceneManager] Loaded model: " << name << " (" << obj->meshCount << " meshes)" << std::endl;
        } else {
            obj->loaded = false;
            obj->meshCount = 0;
            std::cerr << "[SceneManager] Failed to load: " << path << std::endl;
        }
        
        rootObjects.push_back(obj);
        allObjects.push_back(obj);
        return obj;
    }
    
    // Load model as child of parent
    SceneObject* LoadModelAsChild(const std::string& name, const std::string& path, SceneObject* parent) {
        SceneObject* obj = LoadModel(name, path);
        if (obj && parent) {
            // Remove from root, add to parent
            for (auto it = rootObjects.begin(); it != rootObjects.end(); ++it) {
                if (*it == obj) {
                    rootObjects.erase(it);
                    break;
                }
            }
            obj->parent = parent;
            parent->children.push_back(obj);
        }
        return obj;
    }
    
    // Delete object
    void DeleteObject(SceneObject* obj) {
        if (!obj) return;
        
        // Remove from parent
        if (obj->parent) {
            for (auto it = obj->parent->children.begin(); it != obj->parent->children.end(); ++it) {
                if (*it == obj) {
                    obj->parent->children.erase(it);
                    break;
                }
            }
        } else {
            for (auto it = rootObjects.begin(); it != rootObjects.end(); ++it) {
                if (*it == obj) {
                    rootObjects.erase(it);
                    break;
                }
            }
        }
        
        // Remove from allObjects
        for (auto it = allObjects.begin(); it != allObjects.end(); ++it) {
            if (*it == obj) {
                allObjects.erase(it);
                break;
            }
        }
        
        // Delete children recursively
        for (auto child : obj->children) {
            child->parent = nullptr;
            DeleteObject(child);
        }
        
        delete obj;
    }
    
    // ===== TRANSFORM METHODS =====
    
    void SetPosition(SceneObject* obj, float x, float y, float z) {
        if (obj) obj->localTransform.position = glm::vec3(x, y, z);
    }
    
    void SetRotation(SceneObject* obj, float pitch, float yaw, float roll) {
        if (obj) obj->localTransform.rotation = glm::vec3(pitch, yaw, roll);
    }
    
    void SetScale(SceneObject* obj, float x, float y, float z) {
        if (obj) obj->localTransform.scale = glm::vec3(x, y, z);
    }
    
    glm::vec3 GetWorldPosition(SceneObject* obj) {
        if (!obj) return glm::vec3(0);
        UpdateWorldTransform(obj);
        return obj->worldTransform.position;
    }
    
    // ===== RENDERING =====
    
    void RenderAll(Vulkan* vk) {
        if (!vk) return;
        
        // Update all world transforms
        for (auto obj : rootObjects) {
            UpdateWorldTransformRecursive(obj);
        }
        
        // Render all loaded objects
        for (auto obj : allObjects) {
            if (obj->visible && obj->loaded && obj->parser) {
                vk->setModelMatrix(obj->worldTransform.GetMatrix());
                vk->loadModel(obj->parser->getMeshes());
                vk->renderModel();
            }
        }
    }
    
    void RenderObject(SceneObject* obj, Vulkan* vk) {
        if (!vk || !obj) return;
        if (obj->visible && obj->loaded && obj->parser) {
            UpdateWorldTransform(obj);
            vk->setModelMatrix(obj->worldTransform.GetMatrix());
            vk->loadModel(obj->parser->getMeshes());
            vk->renderModel();
        }
    }
    
    // ===== UTILITIES =====
    
    SceneObject* FindByName(const std::string& name) {
        for (auto obj : allObjects) {
            if (obj->name == name) return obj;
        }
        return nullptr;
    }
    
    std::vector<SceneObject*> GetAllObjects() const { return allObjects; }
    std::vector<SceneObject*> GetRootObjects() const { return rootObjects; }
    
    void Clear() {
        for (auto obj : allObjects) {
            delete obj;
        }
        allObjects.clear();
        rootObjects.clear();
    }
    
    void PrintHierarchy() {
        std::cout << "\n=== SCENE HIERARCHY ===" << std::endl;
        for (auto obj : rootObjects) {
            PrintNode(obj, 0);
        }
        std::cout << "=======================\n" << std::endl;
    }
    
private:
    SceneManager() = default;
    ~SceneManager() { Clear(); }
    
    std::vector<SceneObject*> allObjects;
    std::vector<SceneObject*> rootObjects;
    
    void UpdateWorldTransform(SceneObject* obj) {
        if (!obj) return;
        
        if (obj->parent) {
            glm::mat4 parentWorld = obj->parent->worldTransform.GetMatrix();
            glm::mat4 local = obj->localTransform.GetMatrix();
            glm::mat4 world = parentWorld * local;
            
            // Extract position, rotation, scale from matrix
            obj->worldTransform.position = glm::vec3(world[3]);
            // For simplicity, just store the matrix - we'll use GetMatrix() anyway
            obj->worldTransform.position = glm::vec3(world[3]);
            obj->worldTransform.rotation = obj->localTransform.rotation;  // Keep local for now
            obj->worldTransform.scale = obj->localTransform.scale;
        } else {
            obj->worldTransform = obj->localTransform;
        }
    }
    
    void UpdateWorldTransformRecursive(SceneObject* obj) {
        if (!obj) return;
        UpdateWorldTransform(obj);
        for (auto child : obj->children) {
            UpdateWorldTransformRecursive(child);
        }
    }
    
    void PrintNode(SceneObject* obj, int depth) {
        std::string indent(depth * 2, ' ');
        std::string status = obj->loaded ? "[LOADED]" : "[NOT LOADED]";
        std::string type = obj->parser ? "(Model)" : "(Empty)";
        
        std::cout << indent << "|- " << obj->name << " " << type << " " << status;
        if (obj->loaded) {
            std::cout << " [" << obj->meshCount << " meshes]";
        }
        std::cout << std::endl;
        
        for (auto child : obj->children) {
            PrintNode(child, depth + 1);
        }
    }
};

#endif