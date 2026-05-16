// ObjectScene.h
#ifndef OBJECT_SCENE_H
#define OBJECT_SCENE_H

#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct ObjectScene {
    int id;
    std::string name;
    std::string pathOfModel;
    bool isLoaded;
    bool isVisible;
    bool isSelected;
    float posX, posY, posZ;
    float scaleX, scaleY, scaleZ;
    float rotX, rotY, rotZ;
    bool isDirty;
    glm::mat4 worldMatrix;
    
    ObjectScene() : id(0), isLoaded(false), isVisible(true), isSelected(false),
                    posX(0), posY(0), posZ(0),
                    scaleX(1), scaleY(1), scaleZ(1),
                    rotX(0), rotY(0), rotZ(0),
                    isDirty(true), worldMatrix(1.0f) {}
    
    ObjectScene(int _id, const std::string& _name) : id(_id), name(_name), isLoaded(false), isVisible(true), isSelected(false),
        posX(0), posY(0), posZ(0),
        scaleX(1), scaleY(1), scaleZ(1),
        rotX(0), rotY(0), rotZ(0),
        isDirty(true), worldMatrix(1.0f) {}
    
    void setPosition(const glm::vec3& pos) {
        posX = pos.x; posY = pos.y; posZ = pos.z;
        isDirty = true;
    }
    
    void setScale(const glm::vec3& scale) {
        scaleX = scale.x; scaleY = scale.y; scaleZ = scale.z;
        isDirty = true;
    }
    
    void setRotation(const glm::vec3& euler) {
        rotX = euler.x; rotY = euler.y; rotZ = euler.z;
        isDirty = true;
    }
    
    glm::vec3 getPosition() const { return glm::vec3(posX, posY, posZ); }
    glm::vec3 getScale() const { return glm::vec3(scaleX, scaleY, scaleZ); }
    glm::vec3 getRotation() const { return glm::vec3(rotX, rotY, rotZ); }
    
    void updateWorldMatrix() {
        glm::mat4 trans = glm::translate(glm::mat4(1.0f), glm::vec3(posX, posY, posZ));
        glm::mat4 rot = glm::rotate(glm::mat4(1.0f), glm::radians(rotX), glm::vec3(1,0,0)) *
                        glm::rotate(glm::mat4(1.0f), glm::radians(rotY), glm::vec3(0,1,0)) *
                        glm::rotate(glm::mat4(1.0f), glm::radians(rotZ), glm::vec3(0,0,1));
        glm::mat4 scl = glm::scale(glm::mat4(1.0f), glm::vec3(scaleX, scaleY, scaleZ));
        worldMatrix = trans * rot * scl;
        isDirty = false;
    }
    
    glm::mat4 getWorldMatrix() const { return worldMatrix; }
};

#endif