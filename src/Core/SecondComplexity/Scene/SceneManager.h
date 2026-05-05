#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H

#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_RADIANS

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

class ModelParser;

// Центральная точка сцены
constexpr glm::vec3 SCENE_CENTER = glm::vec3(0.0f, 0.0f, 0.0f);
constexpr float MIN_CAMERA_DISTANCE = 0.5f;
constexpr float MAX_CAMERA_DISTANCE = 500.0f;
constexpr float DEFAULT_CAMERA_DISTANCE = 15.0f;
constexpr float DEFAULT_FOV = 60.0f;

struct Transform {
    glm::vec3 position = glm::vec3(0.0f);
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
    
    glm::mat4 getLocalMatrix() const {
        glm::mat4 mat = glm::translate(glm::mat4(1.0f), position);
        mat = mat * glm::toMat4(rotation);
        mat = glm::scale(mat, scale);
        return mat;
    }
    
    void setEulerAngles(float pitch, float yaw, float roll) {
        rotation = glm::quat(glm::vec3(glm::radians(pitch), glm::radians(yaw), glm::radians(roll)));
    }
    
    glm::vec3 getEulerAngles() const {
        glm::vec3 angles = glm::eulerAngles(rotation);
        return glm::vec3(glm::degrees(angles.x), glm::degrees(angles.y), glm::degrees(angles.z));
    }
    
    glm::vec3 getForward() const { return rotation * glm::vec3(0, 0, -1); }
    glm::vec3 getRight() const { return rotation * glm::vec3(1, 0, 0); }
    glm::vec3 getUp() const { return rotation * glm::vec3(0, 1, 0); }
};

struct GlobalTransform {
    glm::mat4 matrix = glm::mat4(1.0f);
    bool dirty = true;
};

struct CameraComponent {
    float fov = DEFAULT_FOV;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
    float aspectRatio = 16.0f / 9.0f;
    
    float yaw = -90.0f;
    float pitch = 30.0f;
    float distance = DEFAULT_CAMERA_DISTANCE;
    bool isOrbital = true;
    glm::vec3 target = SCENE_CENTER;
    
    glm::mat4 viewMatrix = glm::mat4(1.0f);
    glm::mat4 projectionMatrix = glm::mat4(1.0f);
    glm::vec3 position = glm::vec3(0.0f, 0.0f, DEFAULT_CAMERA_DISTANCE);
    bool matricesDirty = true;
    
    float minPitch = -89.0f;
    float maxPitch = 89.0f;
    float minDistance = MIN_CAMERA_DISTANCE;
    float maxDistance = MAX_CAMERA_DISTANCE;
    float minFov = 10.0f;
    float maxFov = 120.0f;
    
    void updateMatrices(float aspect);
    void updatePosition();
    void rotate(float deltaYaw, float deltaPitch);
    void zoom(float delta);
    void setTarget(const glm::vec3& newTarget);
    void setDistance(float newDistance);
    void setFov(float newFov);
    void reset();
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
    
    glm::vec3 boundingBoxMin = glm::vec3(0.0f);
    glm::vec3 boundingBoxMax = glm::vec3(0.0f);
    bool hasBoundingBox = false;
    
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
    
    void calculateBoundingBox();
    glm::vec3 getCenter() const;
    void centerToWorldOrigin();
};

class SceneManager {
public:
    static SceneManager& Instance() {
        static SceneManager instance;
        return instance;
    }
    
    SceneManager(const SceneManager&) = delete;
    void operator=(const SceneManager&) = delete;
    
    ObjectID CreateObject(const std::string& name = "GameObject");
    ObjectID CreateCamera(const std::string& name = "MainCamera");
    void DestroyObject(ObjectID id);
    void DestroyObject(const std::string& name);
    
    SceneObject* GetSceneObject(ObjectID id);
    SceneObject* GetSceneObject(const std::string& name);
    const std::vector<SceneObject*>& GetAllObjects() const { return m_objectsOrdered; }
    
    void SetPosition(ObjectID id, const glm::vec3& pos);
    void SetRotation(ObjectID id, const glm::quat& rot);
    void SetScale(ObjectID id, const glm::vec3& scale);
    
    glm::vec3 GetWorldPosition(ObjectID id);
    glm::mat4 GetWorldMatrix(ObjectID id);
    
    void SetMainCamera(ObjectID cameraID);
    ObjectID GetMainCameraID() const { return m_mainCameraID; }
    SceneObject* GetMainCameraObject();
    CameraComponent* GetMainCamera();
    
    void UpdateCamera(float deltaTime);
    void UpdateCameraAspect(float aspect);
    void RotateCamera(float deltaYaw, float deltaPitch);
    void ZoomCamera(float delta);
    void SetCameraTarget(const glm::vec3& target);
    void SetCameraDistance(float distance);
    void SetCameraFov(float fov);
    void ResetCamera();
    
    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix() const;
    glm::vec3 GetCameraPosition() const;
    glm::vec3 GetCameraTarget() const;
    float GetCameraDistance() const;
    
    void AddModel(const std::string& name, ModelParser* parser);
    void RemoveModel(const std::string& name);
    ModelParser* GetModelParser(const std::string& name);
    
    void CenterModelToWorld(ObjectID id);
    glm::vec3 GetWorldCenter() const { return SCENE_CENTER; }
    
    void Update(float deltaTime);
    void UpdateWorldTransforms();
    void UpdateAllMatrices();
    
    void ClearScene();
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
    std::map<ObjectID, CameraComponent> m_cameras;
    
    ObjectID m_mainCameraID = 0;
    
    ObjectID m_nextID = 1;
    bool m_transformsDirty = true;
};

#endif