// SceneManager.h - ПОЛНЫЙ ИСПРАВЛЕННЫЙ ФАЙЛ
#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "ObjectScene.h"

class ModelParser;
enum class ObjectType { EMPTY, MODEL, CAMERA, LIGHT };

struct SceneObject {
    uint32_t id;
    std::string name;
    ObjectType type;
    bool loaded;
    bool visible;
    bool selected;
    uint32_t meshCount;
    std::string modelPath;
    uint32_t parentID;
    std::vector<uint32_t> childrenIDs;
    ModelParser* parser;
    
    struct Transform {
        glm::vec3 position;
        glm::quat rotation;
        glm::vec3 scale;
        
        Transform() : position(0.0f), rotation(1.0f, 0.0f, 0.0f, 0.0f), scale(1.0f) {}
        
        glm::vec3 getEulerAngles() const {
            glm::vec3 angles;
            angles.x = glm::degrees(glm::pitch(rotation));
            angles.y = glm::degrees(glm::yaw(rotation));
            angles.z = glm::degrees(glm::roll(rotation));
            return angles;
        }
        
        void setEulerAngles(float pitch, float yaw, float roll) {
            rotation = glm::quat(glm::vec3(glm::radians(pitch), glm::radians(yaw), glm::radians(roll)));
        }
    } localTransform;
    
    glm::mat4 worldMatrix;
    
    SceneObject() : id(0), type(ObjectType::EMPTY), loaded(false), visible(true), selected(false),
                    meshCount(0), parentID(0), parser(nullptr) {
        worldMatrix = glm::mat4(1.0f);
    }
    
    void markDirty() {}
};

class SceneManager {
public:
    static SceneManager& Instance() {
        static SceneManager instance;
        return instance;
    }
    
    // Новая система ObjectScene
    int CreateObjectScene(const std::string& name);
    void DestroyObjectScene(int id);
    ObjectScene* GetObjectScene(int id);
    std::vector<ObjectScene>& GetAllObjectsScene() { return m_objectsScene; }
    const std::vector<ObjectScene>& GetAllObjectsScene() const { return m_objectsScene; }
    
    glm::mat4 GetWorldMatrixScene(int id) const;
    glm::vec3 GetWorldPositionScene(int id) const;
    
    void SetPositionScene(int id, float x, float y, float z);
    void SetScaleScene(int id, float x, float y, float z);
    void SetRotationScene(int id, float pitch, float yaw, float roll);
    
    void SetVisibleScene(int id, bool visible);
    void SelectObjectScene(int id);
    void DeselectObjectScene();
    int GetSelectedObjectScene() const { return m_selectedObjectId; }
    
    void UpdateAllMatrices();
    
    bool LoadModelToScene(int id, const std::string& path);
    void SetModelParser(int id, ModelParser* parser);
    ModelParser* GetModelParser(int id) const;
    
    bool SaveScene(const std::string& filename);
    bool LoadScene(const std::string& filename);
    
    // Управление камерой
    void RotateCamera(float dx, float dy);
    void ZoomCamera(float delta);
    void PanCamera(float dx, float dy);
    
    // Старые методы (обратная совместимость)
    uint32_t CreateObject(const std::string& name);
    void DestroyObject(uint32_t id);
    SceneObject* GetSceneObject(uint32_t id);
    const std::vector<SceneObject*>& GetAllObjects() const { return m_objects; }
    
    void SetPosition(uint32_t id, const glm::vec3& pos);
    void SetRotation(uint32_t id, const glm::quat& rot);
    void SetScale(uint32_t id, const glm::vec3& scale);
    
    glm::vec3 GetWorldPosition(uint32_t id);
    glm::mat4 GetWorldMatrix(uint32_t id);
    
    void SelectObject(uint32_t id);
    void DeselectObject();
    SceneObject* GetSelectedObject();
    uint32_t GetSelectedObjectID() const;
    
    void UpdateWorldTransforms();
    
    uint32_t CreateCamera(const std::string& name);
    void SetMainCamera(uint32_t id);
    SceneObject* GetMainCamera();
    uint32_t GetMainCameraID() const { return m_mainCameraID; }
    
    void SetCameraPosition(const glm::vec3& pos);
    void SetCameraTarget(const glm::vec3& target);
    void SetCameraDistance(float distance);
    void SetCameraFov(float fov);
    void SetCameraAspect(float aspect);
    void ResetCamera();
    
    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix() const;
    glm::vec3 GetCameraPosition() const;
    glm::vec3 GetCameraTarget() const;
    float GetCameraDistance() const { return m_cameraDistance; }
    float GetCameraFov() const { return m_cameraFov; }
    void UpdateCameraAspect(float aspect);
    
    void AddModel(const std::string& name, ModelParser* parser);
    void ClearScene();
    
private:
    SceneManager();
    ~SceneManager();
    
    std::vector<ObjectScene> m_objectsScene;
    int m_nextIdScene = 1;
    int m_selectedObjectId = 0;
    std::map<int, ModelParser*> m_modelParsers;
    
    std::vector<SceneObject*> m_objects;
    std::map<uint32_t, SceneObject*> m_objectMap;
    uint32_t m_nextId = 1;
    uint32_t m_selectedID = 0;
    uint32_t m_mainCameraID = 0;
    
    glm::vec3 m_cameraPos;
    glm::vec3 m_cameraTarget;
    float m_cameraDistance;
    float m_cameraFov;
    float m_cameraAspect;
    glm::mat4 m_viewMatrix;
    glm::mat4 m_projMatrix;
    
    void UpdateViewMatrix();
    void UpdateProjMatrix();
    
    void SyncFromOldToNew();
    void SyncFromNewToOld(int id);
};

#endif