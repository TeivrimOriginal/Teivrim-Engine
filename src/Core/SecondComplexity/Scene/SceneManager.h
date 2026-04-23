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
    glm::vec3 rotation = glm::vec3(0.0f);
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
    Transform worldTransform;
    SceneObject* parent;
    std::vector<SceneObject*> children;
    bool visible;
    bool loaded;
    int meshCount;
    
    SceneObject() : parser(nullptr), parent(nullptr), visible(true), loaded(false), meshCount(0) {}
    ~SceneObject() {}
};

class SceneManager {
public:
    static SceneManager& Instance() {
        static SceneManager instance;
        return instance;
    }
    
    SceneObject* CreateEmpty(const std::string& name) {
        SceneObject* obj = new SceneObject();
        obj->name = name;
        obj->parent = nullptr;
        rootObjects.push_back(obj);
        allObjects.push_back(obj);
        return obj;
    }
    
    SceneObject* AddModel(const std::string& name, ModelParser* parser) {
        SceneObject* obj = new SceneObject();
        obj->name = name;
        obj->parser = parser;
        obj->loaded = true;
        obj->meshCount = (int)parser->getMeshes().size();
        obj->parent = nullptr;
        
        rootObjects.push_back(obj);
        allObjects.push_back(obj);
        return obj;
    }
    
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
    
    void DeleteObject(SceneObject* obj) {
        if (!obj) return;
        
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
        
        for (auto it = allObjects.begin(); it != allObjects.end(); ++it) {
            if (*it == obj) {
                allObjects.erase(it);
                break;
            }
        }
        
        for (auto child : obj->children) {
            child->parent = nullptr;
            DeleteObject(child);
        }
        
        delete obj;
    }
    
    void SetPosition(SceneObject* obj, float x, float y, float z) {
        if (obj) obj->localTransform.position = glm::vec3(x, y, z);
    }
    
    void SetRotation(SceneObject* obj, float pitch, float yaw, float roll) {
        if (obj) obj->localTransform.rotation = glm::vec3(pitch, yaw, roll);
    }
    
    void SetScale(SceneObject* obj, float x, float y, float z) {
        if (obj) obj->localTransform.scale = glm::vec3(x, y, z);
    }
    
    void RenderAll(Vulkan* vk) {
        if (!vk) return;
        
        // Устанавливаем трансформы для всех объектов
        for (auto obj : allObjects) {
            if (obj->visible && obj->loaded && obj->parser) {
                vk->setModelTransform(obj->name, obj->localTransform.GetMatrix());
            }
        }
        
        // Рендерим все модели
        vk->renderAllModels();
    }
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
void RenderModel(Vulkan* vk, const std::string& name) {
    if (!vk) return;
    auto obj = FindByName(name);
    if (obj && obj->visible && obj->loaded && obj->parser) {
        vk->setModelTransform(name, obj->localTransform.GetMatrix());
        vk->renderModel(name);
    }
}
private:
    SceneManager() = default;
    ~SceneManager() { Clear(); }
    
    std::vector<SceneObject*> allObjects;
    std::vector<SceneObject*> rootObjects;
};

#endif