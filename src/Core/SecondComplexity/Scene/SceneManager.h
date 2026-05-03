// SceneManager.h - FULL WITH ALL METHODS
#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class ModelParser;

// Структура трансформа
struct Transform {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
    
    glm::mat4 getLocalMatrix() const {
        glm::mat4 mat = glm::translate(glm::mat4(1.0f), position);
        mat = glm::rotate(mat, glm::radians(rotation.x), glm::vec3(1, 0, 0));
        mat = glm::rotate(mat, glm::radians(rotation.y), glm::vec3(0, 1, 0));
        mat = glm::rotate(mat, glm::radians(rotation.z), glm::vec3(0, 0, 1));
        mat = glm::scale(mat, scale);
        return mat;
    }
};

struct GlobalTransform {
    glm::mat4 matrix = glm::mat4(1.0f);
    bool dirty = true;
};

enum class ObjectType {
    EMPTY,
    MODEL,
    LIGHT,
    CAMERA,
    TERRAIN
};

using ObjectID = uint32_t;

class SceneObject {
public:
    ObjectID id;
    std::string name;
    ObjectType type = ObjectType::EMPTY;
    
    ObjectID parentID = 0;
    std::vector<ObjectID> childrenIDs;
    
    Transform localTransform;
    GlobalTransform globalTransform;
    
    bool loaded = false;
    bool visible = true;
    ModelParser* parser = nullptr;
    std::string modelPath;
    uint32_t meshCount = 0;
    
    SceneObject() : id(0) {}
    explicit SceneObject(ObjectID _id) : id(_id) {}
    
    void markDirty() { globalTransform.dirty = true; }
    bool isDirty() const { return globalTransform.dirty; }
    void clean() { globalTransform.dirty = false; }
};

class SceneManager {
public:
    static SceneManager& Instance() {
        static SceneManager instance;
        return instance;
    }
    
    SceneManager(const SceneManager&) = delete;
    void operator=(const SceneManager&) = delete;
    
    // ----- УПРАВЛЕНИЕ ОБЪЕКТАМИ -----
    ObjectID CreateObject(const std::string& name = "GameObject");
    ObjectID CreateCamera(const std::string& name = "MainCamera");
    void DestroyObject(ObjectID id);
    void DestroyObject(const std::string& name);
    
    SceneObject* GetObject(ObjectID id);
    SceneObject* GetObject(const std::string& name);
    const std::vector<SceneObject*>& GetAllObjects() const { return m_objectsOrdered; }
    
    // ----- ТРАНСФОРМЫ -----
    void SetPosition(ObjectID id, const glm::vec3& pos);
    void SetRotation(ObjectID id, const glm::vec3& rot);
    void SetScale(ObjectID id, const glm::vec3& scale);
    void SetTransform(ObjectID id, const Transform& transform);
    
    glm::mat4 GetWorldMatrix(ObjectID id);
    Transform GetWorldTransform(ObjectID id);
    
    // ----- КАМЕРА -----
    void SetMainCamera(ObjectID cameraID);
    ObjectID GetMainCameraID() const { return m_mainCameraID; }
    SceneObject* GetMainCameraObject();
    
    void UpdateCameraAspect(float aspect);
    void UpdateCameraMatrices();
    void UpdateTransforms();
    
    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix() const;
    glm::vec3 GetCameraPosition() const;
    
    // ----- МОДЕЛИ -----
    void AddModel(const std::string& name, ModelParser* parser);
    void RemoveModel(const std::string& name);
    ModelParser* GetModelParser(const std::string& name);
    
    // ----- ОБНОВЛЕНИЕ -----
    void Update(float deltaTime);
    void UpdateWorldTransforms();
    
    // ----- ОЧИСТКА -----
    void ClearScene();
    
    // ----- ОТЛАДКА -----
    void PrintSceneHierarchy();
    size_t GetObjectCount() const { return m_objects.size(); }
    
private:
    SceneManager();
    ~SceneManager();
    
    ObjectID GenerateNewID();
    void UpdateGlobalTransform(ObjectID id);
    void UpdateChildrenTransforms(ObjectID parentID);
    
    std::map<ObjectID, std::unique_ptr<SceneObject>> m_objects;
    std::vector<SceneObject*> m_objectsOrdered;
    std::map<std::string, ObjectID> m_nameToID;
    std::map<std::string, ModelParser*> m_modelParsers;
    
    // Параметры камеры
    ObjectID m_mainCameraID = 0;
    float m_cameraFOV = 45.0f;
    float m_cameraNear = 0.1f;
    float m_cameraFar = 1000.0f;
    float m_cameraAspect = 16.0f / 9.0f;
    bool m_cameraDirty = true;
    
    glm::mat4 m_cachedViewMatrix = glm::mat4(1.0f);
    glm::mat4 m_cachedProjMatrix = glm::mat4(1.0f);
    glm::vec3 m_cachedCameraPos = glm::vec3(0.0f, 50.0f, 150.0f);
    
    ObjectID m_nextID = 1;
    bool m_transformsDirty = true;
};

#endif