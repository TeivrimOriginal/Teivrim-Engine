// SceneManager.cpp - FULL IMPLEMENTATION
#include "SceneManager.h"
#include "../../Render/Parser/parser.h"
#include <iostream>
#include <queue>
#include <fstream>

SceneManager::SceneManager() {
    std::cout << "[SceneManager] Initialized" << std::endl;
}

SceneManager::~SceneManager() {
    ClearScene();
}

ObjectID SceneManager::GenerateNewID() {
    return m_nextID++;
}

ObjectID SceneManager::CreateObject(const std::string& name) {
    ObjectID id = GenerateNewID();
    auto obj = std::make_unique<SceneObject>(id);
    obj->name = name.empty() ? ("Object_" + std::to_string(id)) : name;
    
    m_nameToID[obj->name] = id;
    m_objects[id] = std::move(obj);
    m_objectsOrdered.push_back(m_objects[id].get());
    
    std::cout << "[SceneManager] Created object: " << m_objects[id]->name << " (ID: " << id << ")" << std::endl;
    return id;
}

ObjectID SceneManager::CreateCamera(const std::string& name) {
    ObjectID id = CreateObject(name);
    auto obj = GetObject(id);
    if (obj) {
        obj->type = ObjectType::CAMERA;
    }
    if (m_mainCameraID == 0) {
        SetMainCamera(id);
    }
    return id;
}

void SceneManager::DestroyObject(ObjectID id) {
    auto it = m_objects.find(id);
    if (it == m_objects.end()) return;
    
    for (auto it2 = m_objectsOrdered.begin(); it2 != m_objectsOrdered.end(); ++it2) {
        if ((*it2)->id == id) {
            m_objectsOrdered.erase(it2);
            break;
        }
    }
    
    for (ObjectID childID : it->second->childrenIDs) {
        DestroyObject(childID);
    }
    
    m_nameToID.erase(it->second->name);
    m_objects.erase(it);
    
    if (m_mainCameraID == id) {
        m_mainCameraID = 0;
    }
    
    std::cout << "[SceneManager] Destroyed object ID: " << id << std::endl;
}

void SceneManager::DestroyObject(const std::string& name) {
    auto it = m_nameToID.find(name);
    if (it != m_nameToID.end()) {
        DestroyObject(it->second);
    }
}

SceneObject* SceneManager::GetObject(ObjectID id) {
    auto it = m_objects.find(id);
    return (it != m_objects.end()) ? it->second.get() : nullptr;
}

SceneObject* SceneManager::GetObject(const std::string& name) {
    auto it = m_nameToID.find(name);
    return (it != m_nameToID.end()) ? GetObject(it->second) : nullptr;
}

void SceneManager::SetPosition(ObjectID id, const glm::vec3& pos) {
    SceneObject* obj = GetObject(id);
    if (!obj) return;
    obj->localTransform.position = pos;
    obj->markDirty();
    m_transformsDirty = true;
    
    if (id == m_mainCameraID) {
        m_cameraDirty = true;
    }
}

void SceneManager::SetRotation(ObjectID id, const glm::vec3& rot) {
    SceneObject* obj = GetObject(id);
    if (!obj) return;
    obj->localTransform.rotation = rot;
    obj->markDirty();
    m_transformsDirty = true;
    
    if (id == m_mainCameraID) {
        m_cameraDirty = true;
    }
}

void SceneManager::SetScale(ObjectID id, const glm::vec3& scale) {
    SceneObject* obj = GetObject(id);
    if (!obj) return;
    obj->localTransform.scale = scale;
    obj->markDirty();
    m_transformsDirty = true;
}

void SceneManager::SetTransform(ObjectID id, const Transform& transform) {
    SceneObject* obj = GetObject(id);
    if (!obj) return;
    obj->localTransform = transform;
    obj->markDirty();
    m_transformsDirty = true;
    
    if (id == m_mainCameraID) {
        m_cameraDirty = true;
    }
}

glm::mat4 SceneManager::GetWorldMatrix(ObjectID id) {
    if (m_transformsDirty) {
        UpdateWorldTransforms();
    }
    SceneObject* obj = GetObject(id);
    return obj ? obj->globalTransform.matrix : glm::mat4(1.0f);
}

Transform SceneManager::GetWorldTransform(ObjectID id) {
    glm::mat4 world = GetWorldMatrix(id);
    Transform result;
    result.position = glm::vec3(world[3]);
    return result;
}

void SceneManager::UpdateGlobalTransform(ObjectID id) {
    SceneObject* obj = GetObject(id);
    if (!obj) return;
    if (!obj->isDirty()) return;
    
    if (obj->parentID != 0) {
        SceneObject* parent = GetObject(obj->parentID);
        if (parent) {
            if (parent->isDirty()) {
                UpdateGlobalTransform(parent->id);
            }
            obj->globalTransform.matrix = parent->globalTransform.matrix * obj->localTransform.getLocalMatrix();
        } else {
            obj->globalTransform.matrix = obj->localTransform.getLocalMatrix();
        }
    } else {
        obj->globalTransform.matrix = obj->localTransform.getLocalMatrix();
    }
    
    obj->clean();
}

void SceneManager::UpdateChildrenTransforms(ObjectID parentID) {
    SceneObject* parent = GetObject(parentID);
    if (!parent) return;
    
    for (ObjectID childID : parent->childrenIDs) {
        SceneObject* child = GetObject(childID);
        if (child && child->isDirty()) {
            child->globalTransform.matrix = parent->globalTransform.matrix * child->localTransform.getLocalMatrix();
            child->clean();
            UpdateChildrenTransforms(childID);
        }
    }
}

void SceneManager::UpdateWorldTransforms() {
    for (auto obj : m_objectsOrdered) {
        if (obj->parentID == 0 && obj->isDirty()) {
            UpdateGlobalTransform(obj->id);
            UpdateChildrenTransforms(obj->id);
        }
    }
    m_transformsDirty = false;
}

void SceneManager::SetMainCamera(ObjectID cameraID) {
    SceneObject* obj = GetObject(cameraID);
    if (obj && obj->type == ObjectType::CAMERA) {
        m_mainCameraID = cameraID;
        m_cameraDirty = true;
        std::cout << "[SceneManager] Main camera set to: " << obj->name << std::endl;
    }
}

SceneObject* SceneManager::GetMainCameraObject() {
    return GetObject(m_mainCameraID);
}

void SceneManager::UpdateCameraAspect(float aspect) {
    m_cameraAspect = aspect;
    m_cameraDirty = true;
}

void SceneManager::UpdateCameraMatrices() {
    if (!m_cameraDirty && !m_transformsDirty) return;
    
    SceneObject* camObj = GetMainCameraObject();
    if (!camObj) {
        // Камера по умолчанию
        m_cachedViewMatrix = glm::lookAt(glm::vec3(0, 50, 150), glm::vec3(0, 50, 0), glm::vec3(0, 1, 0));
        m_cachedProjMatrix = glm::perspective(glm::radians(45.0f), m_cameraAspect, m_cameraNear, m_cameraFar);
        m_cachedCameraPos = glm::vec3(0, 50, 150);
        m_cameraDirty = false;
        return;
    }
    
    if (m_transformsDirty) {
        UpdateWorldTransforms();
    }
    
    // View matrix = inverse of world transform
    m_cachedViewMatrix = glm::inverse(camObj->globalTransform.matrix);
    m_cachedCameraPos = glm::vec3(camObj->globalTransform.matrix[3]);
    
    // Projection matrix
    m_cachedProjMatrix = glm::perspective(glm::radians(m_cameraFOV), m_cameraAspect, m_cameraNear, m_cameraFar);
    
    m_cameraDirty = false;
}

void SceneManager::UpdateTransforms() {
    UpdateWorldTransforms();
    UpdateCameraMatrices();
}

glm::mat4 SceneManager::GetViewMatrix() const {
    return m_cachedViewMatrix;
}

glm::mat4 SceneManager::GetProjectionMatrix() const {
    return m_cachedProjMatrix;
}

glm::vec3 SceneManager::GetCameraPosition() const {
    return m_cachedCameraPos;
}

void SceneManager::AddModel(const std::string& name, ModelParser* parser) {
    m_modelParsers[name] = parser;
    
    // Создаём объект сцены для модели
    ObjectID id = CreateObject(name);
    SceneObject* obj = GetObject(id);
    if (obj) {
        obj->type = ObjectType::MODEL;
        obj->parser = parser;
        obj->loaded = true;
        obj->meshCount = parser->getMeshes().size();
    }
    
    std::cout << "[SceneManager] Added model: " << name << std::endl;
}

void SceneManager::RemoveModel(const std::string& name) {
    auto it = m_modelParsers.find(name);
    if (it != m_modelParsers.end()) {
        delete it->second;
        m_modelParsers.erase(it);
    }
    
    ObjectID id = 0;
    auto nameIt = m_nameToID.find(name);
    if (nameIt != m_nameToID.end()) {
        id = nameIt->second;
    }
    
    if (id != 0) {
        DestroyObject(id);
    }
}

ModelParser* SceneManager::GetModelParser(const std::string& name) {
    auto it = m_modelParsers.find(name);
    return (it != m_modelParsers.end()) ? it->second : nullptr;
}

void SceneManager::Update(float deltaTime) {
    // Обновление сцены (анимации, физика и т.д.)
    UpdateTransforms();
}

void SceneManager::ClearScene() {
    for (auto& pair : m_modelParsers) {
        delete pair.second;
    }
    m_modelParsers.clear();
    m_objects.clear();
    m_objectsOrdered.clear();
    m_nameToID.clear();
    m_mainCameraID = 0;
    m_nextID = 1;
    m_transformsDirty = true;
    m_cameraDirty = true;
    
    std::cout << "[SceneManager] Scene cleared" << std::endl;
}

void SceneManager::PrintSceneHierarchy() {
    std::cout << "\n========== SCENE HIERARCHY ==========" << std::endl;
    
    for (auto obj : m_objectsOrdered) {
        if (obj->parentID == 0) {
            std::cout << obj->name << " (ID: " << obj->id;
            if (obj->type == ObjectType::CAMERA) std::cout << ", CAMERA";
            if (obj->type == ObjectType::MODEL) std::cout << ", MODEL";
            std::cout << ")" << std::endl;
            
            for (ObjectID childID : obj->childrenIDs) {
                SceneObject* child = GetObject(childID);
                if (child) {
                    std::cout << "  └─ " << child->name << " (ID: " << child->id << ")" << std::endl;
                }
            }
        }
    }
    std::cout << "=====================================\n" << std::endl;
}