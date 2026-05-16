// SceneManager.cpp - ПОЛНЫЙ ИСПРАВЛЕННЫЙ ФАЙЛ (без дублирования GetAllObjects)
#define GLM_ENABLE_EXPERIMENTAL

#include "SceneManager.h"
#include "../../Render/Parser/parser.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <float.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

SceneManager::SceneManager() 
    : m_cameraPos(0.0f, 50.0f, 150.0f)
    , m_cameraTarget(0.0f, 0.0f, 0.0f)
    , m_cameraDistance(150.0f)
    , m_cameraFov(60.0f)
    , m_cameraAspect(16.0f / 9.0f) {
    UpdateViewMatrix();
    UpdateProjMatrix();
    
    CreateObjectScene("Root");
}

SceneManager::~SceneManager() {
    for (auto obj : m_objects) {
        delete obj;
    }
    m_objects.clear();
    m_objectsScene.clear();
}

// ============================================
// НОВАЯ СИСТЕМА - ObjectScene
// ============================================

int SceneManager::CreateObjectScene(const std::string& name) {
    int id = m_nextIdScene++;
    ObjectScene obj(id, name);
    obj.isLoaded = true;
    obj.isVisible = true;
    m_objectsScene.push_back(obj);
    
    std::cout << "[SceneManager] Created ObjectScene: " << name << " (ID: " << id << ")" << std::endl;
    return id;
}

void SceneManager::DestroyObjectScene(int id) {
    auto it = std::find_if(m_objectsScene.begin(), m_objectsScene.end(),
        [id](const ObjectScene& obj) { return obj.id == id; });
    
    if (it != m_objectsScene.end()) {
        auto parserIt = m_modelParsers.find(id);
        if (parserIt != m_modelParsers.end()) {
            delete parserIt->second;
            m_modelParsers.erase(parserIt);
        }
        
        if (m_selectedObjectId == id) {
            m_selectedObjectId = 0;
        }
        
        m_objectsScene.erase(it);
        std::cout << "[SceneManager] Destroyed ObjectScene ID: " << id << std::endl;
    }
}

ObjectScene* SceneManager::GetObjectScene(int id) {
    for (auto& obj : m_objectsScene) {
        if (obj.id == id) {
            return &obj;
        }
    }
    return nullptr;
}

glm::mat4 SceneManager::GetWorldMatrixScene(int id) const {
    for (const auto& obj : m_objectsScene) {
        if (obj.id == id) {
            return obj.getWorldMatrix();
        }
    }
    return glm::mat4(1.0f);
}

glm::vec3 SceneManager::GetWorldPositionScene(int id) const {
    for (const auto& obj : m_objectsScene) {
        if (obj.id == id) {
            return obj.getPosition();
        }
    }
    return glm::vec3(0.0f);
}

void SceneManager::SetPositionScene(int id, float x, float y, float z) {
    ObjectScene* obj = GetObjectScene(id);
    if (obj) {
        obj->setPosition(glm::vec3(x, y, z));
        SyncFromNewToOld(id);
        std::cout << "[SceneManager] SetPositionScene ID " << id << ": (" << x << ", " << y << ", " << z << ")" << std::endl;
    }
}

void SceneManager::SetScaleScene(int id, float x, float y, float z) {
    ObjectScene* obj = GetObjectScene(id);
    if (obj) {
        obj->setScale(glm::vec3(x, y, z));
        SyncFromNewToOld(id);
    }
}

void SceneManager::SetRotationScene(int id, float pitch, float yaw, float roll) {
    ObjectScene* obj = GetObjectScene(id);
    if (obj) {
        obj->setRotation(glm::vec3(pitch, yaw, roll));
        SyncFromNewToOld(id);
    }
}

void SceneManager::SetVisibleScene(int id, bool visible) {
    ObjectScene* obj = GetObjectScene(id);
    if (obj) {
        obj->isVisible = visible;
        SyncFromNewToOld(id);
    }
}

void SceneManager::SelectObjectScene(int id) {
    if (m_selectedObjectId != 0) {
        ObjectScene* prev = GetObjectScene(m_selectedObjectId);
        if (prev) prev->isSelected = false;
    }
    
    m_selectedObjectId = id;
    ObjectScene* selected = GetObjectScene(id);
    if (selected) {
        selected->isSelected = true;
        SyncFromNewToOld(id);
        std::cout << "[SceneManager] Selected ObjectScene ID: " << id << " (" << selected->name << ")" << std::endl;
    }
}

void SceneManager::DeselectObjectScene() {
    if (m_selectedObjectId != 0) {
        ObjectScene* prev = GetObjectScene(m_selectedObjectId);
        if (prev) prev->isSelected = false;
    }
    m_selectedObjectId = 0;
}

void SceneManager::UpdateAllMatrices() {
    for (auto& obj : m_objectsScene) {
        if (obj.isDirty) {
            obj.updateWorldMatrix();
        }
    }
}

bool SceneManager::LoadModelToScene(int id, const std::string& path) {
    ObjectScene* obj = GetObjectScene(id);
    if (!obj) return false;
    
    obj->pathOfModel = path;
    obj->isLoaded = true;
    
    std::cout << "[SceneManager] LoadModelToScene ID " << id << ": " << path << std::endl;
    return true;
}

void SceneManager::SetModelParser(int id, ModelParser* parser) {
    m_modelParsers[id] = parser;
    ObjectScene* obj = GetObjectScene(id);
    if (obj) {
        obj->isLoaded = (parser != nullptr);
    }
}

ModelParser* SceneManager::GetModelParser(int id) const {
    auto it = m_modelParsers.find(id);
    if (it != m_modelParsers.end()) {
        return it->second;
    }
    return nullptr;
}

bool SceneManager::SaveScene(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "[SceneManager] Failed to save scene to: " << filename << std::endl;
        return false;
    }
    
    file << "{\n";
    file << "  \"version\": 1,\n";
    file << "  \"objects\": [\n";
    
    for (size_t i = 0; i < m_objectsScene.size(); i++) {
        const auto& obj = m_objectsScene[i];
        file << "    {\n";
        file << "      \"id\": " << obj.id << ",\n";
        file << "      \"name\": \"" << obj.name << "\",\n";
        file << "      \"pathOfModel\": \"" << obj.pathOfModel << "\",\n";
        file << "      \"isLoaded\": " << (obj.isLoaded ? "true" : "false") << ",\n";
        file << "      \"isVisible\": " << (obj.isVisible ? "true" : "false") << ",\n";
        file << "      \"position\": [" << obj.posX << ", " << obj.posY << ", " << obj.posZ << "],\n";
        file << "      \"scale\": [" << obj.scaleX << ", " << obj.scaleY << ", " << obj.scaleZ << "],\n";
        file << "      \"rotation\": [" << obj.rotX << ", " << obj.rotY << ", " << obj.rotZ << "]\n";
        file << "    }" << (i < m_objectsScene.size() - 1 ? "," : "") << "\n";
    }
    
    file << "  ]\n";
    file << "}\n";
    file.close();
    
    std::cout << "[SceneManager] Scene saved to: " << filename << std::endl;
    return true;
}

bool SceneManager::LoadScene(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "[SceneManager] Failed to load scene from: " << filename << std::endl;
        return false;
    }
    
    m_objectsScene.clear();
    m_modelParsers.clear();
    m_nextIdScene = 1;
    m_selectedObjectId = 0;
    
    CreateObjectScene("Root");
    
    std::cout << "[SceneManager] Scene loaded from: " << filename << std::endl;
    file.close();
    return true;
}

void SceneManager::SyncFromOldToNew() {
    for (const auto& oldObj : m_objects) {
        if (oldObj) {
            ObjectScene* newObj = GetObjectScene(oldObj->id);
            if (newObj) {
                newObj->name = oldObj->name;
                newObj->isVisible = oldObj->visible;
                newObj->posX = oldObj->localTransform.position.x;
                newObj->posY = oldObj->localTransform.position.y;
                newObj->posZ = oldObj->localTransform.position.z;
                newObj->scaleX = oldObj->localTransform.scale.x;
                newObj->scaleY = oldObj->localTransform.scale.y;
                newObj->scaleZ = oldObj->localTransform.scale.z;
                
                glm::vec3 euler = oldObj->localTransform.getEulerAngles();
                newObj->rotX = euler.x;
                newObj->rotY = euler.y;
                newObj->rotZ = euler.z;
                
                newObj->pathOfModel = oldObj->modelPath;
                newObj->isLoaded = oldObj->loaded;
                newObj->updateWorldMatrix();
            }
        }
    }
}

void SceneManager::SyncFromNewToOld(int id) {
    ObjectScene* newObj = GetObjectScene(id);
    if (!newObj) return;
    
    auto it = m_objectMap.find(id);
    if (it != m_objectMap.end() && it->second) {
        SceneObject* oldObj = it->second;
        oldObj->name = newObj->name;
        oldObj->visible = newObj->isVisible;
        oldObj->selected = newObj->isSelected;
        oldObj->localTransform.position = newObj->getPosition();
        oldObj->localTransform.scale = newObj->getScale();
        oldObj->localTransform.setEulerAngles(newObj->rotX, newObj->rotY, newObj->rotZ);
        oldObj->modelPath = newObj->pathOfModel;
        oldObj->loaded = newObj->isLoaded;
        oldObj->worldMatrix = newObj->getWorldMatrix();
    }
}

// ============================================
// СТАРЫЕ МЕТОДЫ (обратная совместимость)
// ============================================

uint32_t SceneManager::CreateObject(const std::string& name) {
    uint32_t id = m_nextId++;
    SceneObject* obj = new SceneObject();
    obj->id = id;
    obj->name = name;
    obj->type = ObjectType::EMPTY;
    obj->loaded = false;
    obj->visible = true;
    obj->selected = false;
    obj->meshCount = 0;
    obj->parentID = 0;
    obj->parser = nullptr;
    obj->worldMatrix = glm::mat4(1.0f);
    
    m_objects.push_back(obj);
    m_objectMap[id] = obj;
    
    int newId = CreateObjectScene(name);
    if (newId != id) {
        for (auto& sceneObj : m_objectsScene) {
            if (sceneObj.id == newId) {
                sceneObj.id = id;
                break;
            }
        }
    }
    
    std::cout << "[SceneManager] Created object: " << name << " (ID: " << id << ")" << std::endl;
    return id;
}

void SceneManager::DestroyObject(uint32_t id) {
    auto it = m_objectMap.find(id);
    if (it != m_objectMap.end()) {
        delete it->second;
        m_objectMap.erase(it);
        
        auto vecIt = std::find_if(m_objects.begin(), m_objects.end(),
            [id](SceneObject* obj) { return obj->id == id; });
        if (vecIt != m_objects.end()) {
            m_objects.erase(vecIt);
        }
        
        if (m_selectedID == id) {
            m_selectedID = 0;
        }
    }
    
    DestroyObjectScene(id);
}

SceneObject* SceneManager::GetSceneObject(uint32_t id) {
    auto it = m_objectMap.find(id);
    if (it != m_objectMap.end()) {
        return it->second;
    }
    return nullptr;
}

// GetAllObjects() уже определён в .h файле как inline, поэтому НЕ определяем его здесь повторно

void SceneManager::SetPosition(uint32_t id, const glm::vec3& pos) {
    SceneObject* obj = GetSceneObject(id);
    if (obj) {
        obj->localTransform.position = pos;
        SetPositionScene(id, pos.x, pos.y, pos.z);
    }
}

void SceneManager::SetRotation(uint32_t id, const glm::quat& rot) {
    SceneObject* obj = GetSceneObject(id);
    if (obj) {
        obj->localTransform.rotation = rot;
        glm::vec3 euler = obj->localTransform.getEulerAngles();
        SetRotationScene(id, euler.x, euler.y, euler.z);
    }
}

void SceneManager::SetScale(uint32_t id, const glm::vec3& scale) {
    SceneObject* obj = GetSceneObject(id);
    if (obj) {
        obj->localTransform.scale = scale;
        SetScaleScene(id, scale.x, scale.y, scale.z);
    }
}

glm::vec3 SceneManager::GetWorldPosition(uint32_t id) {
    return GetWorldPositionScene(id);
}

glm::mat4 SceneManager::GetWorldMatrix(uint32_t id) {
    return GetWorldMatrixScene(id);
}

void SceneManager::SelectObject(uint32_t id) {
    if (m_selectedID != 0) {
        SceneObject* prev = GetSceneObject(m_selectedID);
        if (prev) prev->selected = false;
    }
    
    m_selectedID = id;
    SceneObject* selected = GetSceneObject(id);
    if (selected) {
        selected->selected = true;
    }
    
    SelectObjectScene(id);
}

void SceneManager::DeselectObject() {
    if (m_selectedID != 0) {
        SceneObject* prev = GetSceneObject(m_selectedID);
        if (prev) prev->selected = false;
    }
    m_selectedID = 0;
    DeselectObjectScene();
}

SceneObject* SceneManager::GetSelectedObject() {
    if (m_selectedID != 0) {
        return GetSceneObject(m_selectedID);
    }
    return nullptr;
}

uint32_t SceneManager::GetSelectedObjectID() const {
    return m_selectedID;
}

void SceneManager::UpdateWorldTransforms() {
    UpdateAllMatrices();
}

uint32_t SceneManager::CreateCamera(const std::string& name) {
    uint32_t id = CreateObject(name);
    SceneObject* obj = GetSceneObject(id);
    if (obj) {
        obj->type = ObjectType::CAMERA;
    }
    
    if (m_mainCameraID == 0) {
        m_mainCameraID = id;
    }
    
    return id;
}

void SceneManager::SetMainCamera(uint32_t id) {
    m_mainCameraID = id;
}

SceneObject* SceneManager::GetMainCamera() {
    return GetSceneObject(m_mainCameraID);
}

void SceneManager::SetCameraPosition(const glm::vec3& pos) {
    m_cameraPos = pos;
    UpdateViewMatrix();
}

void SceneManager::SetCameraTarget(const glm::vec3& target) {
    m_cameraTarget = target;
    UpdateViewMatrix();
}

void SceneManager::SetCameraDistance(float distance) {
    m_cameraDistance = distance;
    
    glm::vec3 dir = glm::normalize(m_cameraPos - m_cameraTarget);
    m_cameraPos = m_cameraTarget + dir * m_cameraDistance;
    UpdateViewMatrix();
}

void SceneManager::SetCameraFov(float fov) {
    m_cameraFov = fov;
    UpdateProjMatrix();
}

void SceneManager::SetCameraAspect(float aspect) {
    m_cameraAspect = aspect;
    UpdateProjMatrix();
}

void SceneManager::ResetCamera() {
    m_cameraPos = glm::vec3(0.0f, 50.0f, 150.0f);
    m_cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
    m_cameraDistance = 150.0f;
    m_cameraFov = 60.0f;
    UpdateViewMatrix();
    UpdateProjMatrix();
}

glm::mat4 SceneManager::GetViewMatrix() const {
    return m_viewMatrix;
}

glm::mat4 SceneManager::GetProjectionMatrix() const {
    return m_projMatrix;
}

glm::vec3 SceneManager::GetCameraPosition() const {
    return m_cameraPos;
}

glm::vec3 SceneManager::GetCameraTarget() const {
    return m_cameraTarget;
}

void SceneManager::UpdateCameraAspect(float aspect) {
    SetCameraAspect(aspect);
}

void SceneManager::UpdateViewMatrix() {
    m_viewMatrix = glm::lookAt(m_cameraPos, m_cameraTarget, glm::vec3(0.0f, 1.0f, 0.0f));
}

void SceneManager::UpdateProjMatrix() {
    m_projMatrix = glm::perspective(glm::radians(m_cameraFov), m_cameraAspect, 0.1f, 1000.0f);
}

void SceneManager::AddModel(const std::string& name, ModelParser* parser) {
    uint32_t id = CreateObject(name);
    SceneObject* obj = GetSceneObject(id);
    if (obj) {
        obj->type = ObjectType::MODEL;
        obj->parser = parser;
        obj->loaded = true;
        obj->meshCount = (uint32_t)parser->getMeshes().size();
        obj->modelPath = "loaded";
    }
    
    SetModelParser(id, parser);
}

void SceneManager::ClearScene() {
    for (auto obj : m_objects) {
        delete obj;
    }
    m_objects.clear();
    m_objectMap.clear();
    m_objectsScene.clear();
    m_modelParsers.clear();
    m_nextId = 1;
    m_nextIdScene = 1;
    m_selectedID = 0;
    m_selectedObjectId = 0;
    
    CreateObjectScene("Root");
}

// ============================================
// МЕТОДЫ УПРАВЛЕНИЯ КАМЕРОЙ
// ============================================

void SceneManager::RotateCamera(float dx, float dy) {
    float sensitivity = 0.005f;
    dx *= sensitivity;
    dy *= sensitivity;
    
    glm::vec3 dir = m_cameraPos - m_cameraTarget;
    float radius = glm::length(dir);
    float theta = atan2(dir.x, dir.z);
    float phi = acos(dir.y / radius);
    
    theta += dx;
    phi += dy;
    
    phi = glm::clamp(phi, 0.01f, glm::pi<float>() - 0.01f);
    
    dir.x = radius * sin(phi) * sin(theta);
    dir.y = radius * cos(phi);
    dir.z = radius * sin(phi) * cos(theta);
    
    m_cameraPos = m_cameraTarget + dir;
    UpdateViewMatrix();
}

void SceneManager::ZoomCamera(float delta) {
    float sensitivity = 1.0f;
    m_cameraDistance -= delta * sensitivity;
    m_cameraDistance = glm::clamp(m_cameraDistance, 5.0f, 500.0f);
    
    glm::vec3 dir = glm::normalize(m_cameraPos - m_cameraTarget);
    m_cameraPos = m_cameraTarget + dir * m_cameraDistance;
    UpdateViewMatrix();
}

void SceneManager::PanCamera(float dx, float dy) {
    float sensitivity = 0.01f;
    glm::vec3 right = glm::normalize(glm::cross(m_cameraPos - m_cameraTarget, glm::vec3(0, 1, 0)));
    glm::vec3 up = glm::normalize(glm::cross(right, m_cameraPos - m_cameraTarget));
    
    m_cameraTarget -= right * dx * sensitivity;
    m_cameraTarget += up * dy * sensitivity;
    m_cameraPos -= right * dx * sensitivity;
    m_cameraPos += up * dy * sensitivity;
    
    UpdateViewMatrix();
}