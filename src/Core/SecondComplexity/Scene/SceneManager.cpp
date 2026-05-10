// SceneManager.cpp
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_RADIANS

#include "../../Render/Parser/parser.h"
#include "SceneManager.h"
#include <iostream>
#include <cmath>

// CameraComponent implementation
void CameraComponent::updateMatrices(float aspect) {
    aspectRatio = aspect;
    
    if (isOrbital) {
        viewMatrix = glm::lookAt(position, target, glm::vec3(0, 1, 0));
    } else {
        glm::quat rot = glm::quat(glm::vec3(glm::radians(pitch), glm::radians(yaw), 0.0f));
        glm::vec3 forward = rot * glm::vec3(0, 0, -1);
        glm::vec3 up = rot * glm::vec3(0, 1, 0);
        viewMatrix = glm::lookAt(position, position + forward, up);
    }
    
    projectionMatrix = glm::perspective(glm::radians(fov), aspect, nearPlane, farPlane);
    matricesDirty = false;
}

void CameraComponent::updatePosition() {
    if (isOrbital) {
        float radYaw = glm::radians(yaw);
        float radPitch = glm::radians(pitch);
        
        float x = distance * cos(radPitch) * cos(radYaw);
        float y = distance * sin(radPitch);
        float z = distance * cos(radPitch) * sin(radYaw);
        
        position = target + glm::vec3(x, y, z);
    }
    matricesDirty = true;
}

void CameraComponent::rotate(float deltaYaw, float deltaPitch) {
    yaw += deltaYaw;
    pitch += deltaPitch;
    
    if (pitch > maxPitch) pitch = maxPitch;
    if (pitch < minPitch) pitch = minPitch;
    
    if (yaw > 360.0f) yaw -= 360.0f;
    if (yaw < -360.0f) yaw += 360.0f;
    
    updatePosition();
}

void CameraComponent::zoom(float delta) {
    distance -= delta * distance * 0.1f;
    if (distance < minDistance) distance = minDistance;
    if (distance > maxDistance) distance = maxDistance;
    updatePosition();
}

void CameraComponent::setTarget(const glm::vec3& newTarget) {
    target = newTarget;
    updatePosition();
}

void CameraComponent::setDistance(float newDistance) {
    distance = newDistance;
    if (distance < minDistance) distance = minDistance;
    if (distance > maxDistance) distance = maxDistance;
    updatePosition();
}

void CameraComponent::setFov(float newFov) {
    fov = newFov;
    if (fov < minFov) fov = minFov;
    if (fov > maxFov) fov = maxFov;
    matricesDirty = true;
}

void CameraComponent::reset() {
    yaw = -90.0f;
    pitch = 30.0f;
    distance = DEFAULT_CAMERA_DISTANCE;
    target = SCENE_CENTER;
    fov = DEFAULT_FOV;
    updatePosition();
    matricesDirty = true;
}

// SceneObject implementation
void SceneObject::calculateBoundingBox() {
    if (!parser || parser->getMeshes().empty()) return;
    
    boundingBoxMin = glm::vec3(FLT_MAX);
    boundingBoxMax = glm::vec3(-FLT_MAX);
    
    for (const auto& mesh : parser->getMeshes()) {
        for (const auto& vertex : mesh.vertices) {
            glm::vec3 pos(vertex.position[0], vertex.position[1], vertex.position[2]);
            boundingBoxMin = glm::min(boundingBoxMin, pos);
            boundingBoxMax = glm::max(boundingBoxMax, pos);
        }
    }
    
    hasBoundingBox = true;
}

glm::vec3 SceneObject::getCenter() const {
    if (!hasBoundingBox) return glm::vec3(0.0f);
    return (boundingBoxMin + boundingBoxMax) * 0.5f;
}

void SceneObject::centerToWorldOrigin() {
    if (!hasBoundingBox) calculateBoundingBox();
    glm::vec3 center = getCenter();
    localTransform.position = -center;
    markDirty();
}

// SceneManager implementation
SceneManager::SceneManager() {
    std::cout << "[SceneManager] Initialized with scene center at (0,0,0)" << std::endl;
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
    auto obj = GetSceneObject(id);
    if (obj) {
        obj->type = ObjectType::CAMERA;
        CameraComponent camera;
        camera.target = SCENE_CENTER;
        camera.updatePosition();
        m_cameras[id] = camera;
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
    m_cameras.erase(id);
    
    if (m_selectedObjectID == id) {
        DeselectObject();
    }
    
    if (m_mainCameraID == id) {
        m_mainCameraID = 0;
    }
    
    m_objects.erase(it);
    
    std::cout << "[SceneManager] Destroyed object ID: " << id << std::endl;
}

void SceneManager::DestroyObject(const std::string& name) {
    auto it = m_nameToID.find(name);
    if (it != m_nameToID.end()) {
        DestroyObject(it->second);
    }
}

SceneObject* SceneManager::GetSceneObject(ObjectID id) {
    auto it = m_objects.find(id);
    return (it != m_objects.end()) ? it->second.get() : nullptr;
}

const SceneObject* SceneManager::GetSceneObject(ObjectID id) const {
    auto it = m_objects.find(id);
    return (it != m_objects.end()) ? it->second.get() : nullptr;
}

SceneObject* SceneManager::GetSceneObject(const std::string& name) {
    auto it = m_nameToID.find(name);
    return (it != m_nameToID.end()) ? GetSceneObject(it->second) : nullptr;
}

// Выбор объекта
void SceneManager::SelectObject(ObjectID id) {
    // Снимаем выделение с предыдущего
    if (m_selectedObjectID != 0) {
        SceneObject* prev = GetSceneObject(m_selectedObjectID);
        if (prev) {
            prev->selected = false;
        }
    }
    
    m_selectedObjectID = id;
    
    SceneObject* obj = GetSceneObject(id);
    if (obj) {
        obj->selected = true;
        std::cout << "[SceneManager] Selected object: " << obj->name << " (ID: " << id << ")" << std::endl;
    }
}

void SceneManager::SelectObject(const std::string& name) {
    auto it = m_nameToID.find(name);
    if (it != m_nameToID.end()) {
        SelectObject(it->second);
    }
}

void SceneManager::DeselectObject() {
    if (m_selectedObjectID != 0) {
        SceneObject* prev = GetSceneObject(m_selectedObjectID);
        if (prev) {
            prev->selected = false;
        }
    }
    m_selectedObjectID = 0;
    std::cout << "[SceneManager] Deselected object" << std::endl;
}

SceneObject* SceneManager::GetSelectedObject() {
    return GetSceneObject(m_selectedObjectID);
}

const SceneObject* SceneManager::GetSelectedObject() const {
    auto it = m_objects.find(m_selectedObjectID);
    return (it != m_objects.end()) ? it->second.get() : nullptr;
}

bool SceneManager::IsObjectSelected(ObjectID id) const {
    return m_selectedObjectID == id;
}

void SceneManager::SetPosition(ObjectID id, const glm::vec3& pos) {
    SceneObject* obj = GetSceneObject(id);
    if (!obj) return;
    obj->localTransform.position = pos;
    obj->markDirty();
    m_transformsDirty = true;
}

void SceneManager::SetRotation(ObjectID id, const glm::quat& rot) {
    SceneObject* obj = GetSceneObject(id);
    if (!obj) return;
    obj->localTransform.rotation = rot;
    obj->markDirty();
    m_transformsDirty = true;
}

void SceneManager::SetScale(ObjectID id, const glm::vec3& scale) {
    SceneObject* obj = GetSceneObject(id);
    if (!obj) return;
    obj->localTransform.scale = scale;
    obj->markDirty();
    m_transformsDirty = true;
}

glm::vec3 SceneManager::GetWorldPosition(ObjectID id) {
    glm::mat4 world = GetWorldMatrix(id);
    return glm::vec3(world[3]);
}

glm::mat4 SceneManager::GetWorldMatrix(ObjectID id) {
    if (m_transformsDirty) {
        UpdateWorldTransforms();
    }
    SceneObject* obj = GetSceneObject(id);
    return obj ? obj->globalTransform.matrix : glm::mat4(1.0f);
}

void SceneManager::UpdateGlobalTransform(ObjectID id) {
    SceneObject* obj = GetSceneObject(id);
    if (!obj) return;
    if (!obj->isDirty()) return;
    
    if (obj->parentID != 0) {
        SceneObject* parent = GetSceneObject(obj->parentID);
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
    SceneObject* parent = GetSceneObject(parentID);
    if (!parent) return;
    
    for (ObjectID childID : parent->childrenIDs) {
        SceneObject* child = GetSceneObject(childID);
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
    SceneObject* obj = GetSceneObject(cameraID);
    if (obj && obj->type == ObjectType::CAMERA) {
        m_mainCameraID = cameraID;
        std::cout << "[SceneManager] Main camera set to: " << obj->name << std::endl;
    }
}

SceneObject* SceneManager::GetMainCameraObject() {
    return GetSceneObject(m_mainCameraID);
}

CameraComponent* SceneManager::GetMainCamera() {
    auto it = m_cameras.find(m_mainCameraID);
    return (it != m_cameras.end()) ? &it->second : nullptr;
}

void SceneManager::UpdateCamera(float deltaTime) {
    CameraComponent* cam = GetMainCamera();
    if (!cam) return;
    cam->updatePosition();
    cam->matricesDirty = true;
}

void SceneManager::UpdateCameraAspect(float aspect) {
    CameraComponent* cam = GetMainCamera();
    if (cam) {
        cam->updateMatrices(aspect);
    }
}

void SceneManager::RotateCamera(float deltaYaw, float deltaPitch) {
    CameraComponent* cam = GetMainCamera();
    if (cam) {
        cam->rotate(deltaYaw, deltaPitch);
    }
}

void SceneManager::ZoomCamera(float delta) {
    CameraComponent* cam = GetMainCamera();
    if (cam) {
        cam->zoom(delta);
    }
}

void SceneManager::SetCameraTarget(const glm::vec3& target) {
    CameraComponent* cam = GetMainCamera();
    if (cam) {
        cam->setTarget(target);
    }
}

void SceneManager::SetCameraDistance(float distance) {
    CameraComponent* cam = GetMainCamera();
    if (cam) {
        cam->setDistance(distance);
    }
}

void SceneManager::SetCameraFov(float fov) {
    CameraComponent* cam = GetMainCamera();
    if (cam) {
        cam->setFov(fov);
    }
}

void SceneManager::ResetCamera() {
    CameraComponent* cam = GetMainCamera();
    if (cam) {
        cam->reset();
    }
}

glm::mat4 SceneManager::GetViewMatrix() const {
    auto it = m_cameras.find(m_mainCameraID);
    if (it != m_cameras.end()) {
        return it->second.viewMatrix;
    }
    return glm::lookAt(glm::vec3(0, 10, 20), SCENE_CENTER, glm::vec3(0, 1, 0));
}

glm::mat4 SceneManager::GetProjectionMatrix() const {
    auto it = m_cameras.find(m_mainCameraID);
    if (it != m_cameras.end()) {
        return it->second.projectionMatrix;
    }
    return glm::perspective(glm::radians(60.0f), 16.0f/9.0f, 0.1f, 1000.0f);
}

glm::vec3 SceneManager::GetCameraPosition() const {
    auto it = m_cameras.find(m_mainCameraID);
    if (it != m_cameras.end()) {
        return it->second.position;
    }
    return glm::vec3(0, 10, 20);
}

glm::vec3 SceneManager::GetCameraTarget() const {
    auto it = m_cameras.find(m_mainCameraID);
    if (it != m_cameras.end()) {
        return it->second.target;
    }
    return SCENE_CENTER;
}

float SceneManager::GetCameraDistance() const {
    auto it = m_cameras.find(m_mainCameraID);
    if (it != m_cameras.end()) {
        return it->second.distance;
    }
    return DEFAULT_CAMERA_DISTANCE;
}

void SceneManager::AddModel(const std::string& name, ModelParser* parser) {
    m_modelParsers[name] = parser;
    
    ObjectID id = CreateObject(name);
    SceneObject* obj = GetSceneObject(id);
    if (obj) {
        obj->type = ObjectType::MODEL;
        obj->parser = parser;
        obj->loaded = true;
        obj->meshCount = (uint32_t)parser->getMeshes().size();
        
        obj->calculateBoundingBox();
        obj->centerToWorldOrigin();
    }
    
    std::cout << "[SceneManager] Added model: " << name << " centered at world origin" << std::endl;
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

void SceneManager::CenterModelToWorld(ObjectID id) {
    SceneObject* obj = GetSceneObject(id);
    if (obj) {
        obj->centerToWorldOrigin();
    }
}

void SceneManager::Update(float deltaTime) {
    UpdateWorldTransforms();
}

void SceneManager::UpdateAllMatrices() {
    UpdateWorldTransforms();
    for (auto& pair : m_cameras) {
        pair.second.updateMatrices(pair.second.aspectRatio);
    }
}

void SceneManager::ClearScene() {
    for (auto& pair : m_modelParsers) {
        delete pair.second;
    }
    m_modelParsers.clear();
    m_objects.clear();
    m_objectsOrdered.clear();
    m_nameToID.clear();
    m_cameras.clear();
    m_mainCameraID = 0;
    m_selectedObjectID = 0;
    m_nextID = 1;
    m_transformsDirty = true;
    
    std::cout << "[SceneManager] Scene cleared" << std::endl;
}

void SceneManager::PrintSceneHierarchy() {
    std::cout << "\n========== SCENE HIERARCHY ==========" << std::endl;
    std::cout << "Scene Center: (0, 0, 0)" << std::endl;
    
    for (auto obj : m_objectsOrdered) {
        if (obj->parentID == 0) {
            std::cout << obj->name << " (ID: " << obj->id;
            if (obj->type == ObjectType::CAMERA) std::cout << ", CAMERA";
            if (obj->type == ObjectType::MODEL) std::cout << ", MODEL";
            if (obj->selected) std::cout << ", SELECTED";
            std::cout << ")" << std::endl;
            std::cout << "  Position: (" << obj->localTransform.position.x << ", " 
                      << obj->localTransform.position.y << ", " << obj->localTransform.position.z << ")" << std::endl;
            
            for (ObjectID childID : obj->childrenIDs) {
                SceneObject* child = GetSceneObject(childID);
                if (child) {
                    std::cout << "  └─ " << child->name << " (ID: " << child->id << ")" << std::endl;
                }
            }
        }
    }
    
    if (m_selectedObjectID != 0) {
        const SceneObject* selected = GetSelectedObject();
        if (selected) {
            std::cout << "\nSelected: " << selected->name << std::endl;
        }
    }
    
    CameraComponent* cam = GetMainCamera();
    if (cam) {
        std::cout << "\nCamera: FOV=" << cam->fov 
                  << ", Distance=" << cam->distance
                  << ", Target=(" << cam->target.x << ", " << cam->target.y << ", " << cam->target.z << ")" << std::endl;
    }
    
    std::cout << "=====================================\n" << std::endl;
}