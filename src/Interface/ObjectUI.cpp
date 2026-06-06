// ObjectUI.cpp
#include "ObjectUI.h"
#include "../Core/SecondComplexity/Scene/SceneManager.h"
#include "../Core/Render/Vulkan/Vulkan.h"
#include <iostream>

ObjectUI::ObjectUI() 
    : m_activeFieldIndex(-1)
    , m_isEditing(false)
    , m_vulkan(nullptr)
{
    for (int i = 0; i < 9; i++) {
        m_fieldValues.push_back("0");
        m_fieldOriginalValues.push_back("0");
    }
}

ObjectUI::~ObjectUI() {}

void ObjectUI::updateFieldValuesFromObject() {
    auto& sm = SceneManager::Instance();
    int selectedId = sm.GetSelectedObjectScene();
    ObjectScene* selectedScene = (selectedId != 0) ? sm.GetObjectScene(selectedId) : nullptr;
    
    if (!selectedScene) return;
    
    glm::vec3 worldPos = selectedScene->getPosition();
    glm::vec3 worldRot = selectedScene->getRotation();
    glm::vec3 worldScale = selectedScene->getScale();
    
    char buf[32];
    sprintf_s(buf, "%.3f", worldPos.x); m_fieldValues[0] = buf;
    sprintf_s(buf, "%.3f", worldPos.y); m_fieldValues[1] = buf;
    sprintf_s(buf, "%.3f", worldPos.z); m_fieldValues[2] = buf;
    
    sprintf_s(buf, "%.1f", worldRot.x); m_fieldValues[3] = buf;
    sprintf_s(buf, "%.1f", worldRot.y); m_fieldValues[4] = buf;
    sprintf_s(buf, "%.1f", worldRot.z); m_fieldValues[5] = buf;
    
    sprintf_s(buf, "%.3f", worldScale.x); m_fieldValues[6] = buf;
    sprintf_s(buf, "%.3f", worldScale.y); m_fieldValues[7] = buf;
    sprintf_s(buf, "%.3f", worldScale.z); m_fieldValues[8] = buf;
}

void ObjectUI::render(RenderUI& r, int w, int h, PanelManager& panels) {
    Panel* inspector = panels.getPanel("Inspector");
    if (!inspector || !inspector->visible || inspector->collapsed) return;
    
    auto& sm = SceneManager::Instance();
    int selectedId = sm.GetSelectedObjectScene();
    ObjectScene* selectedScene = (selectedId != 0) ? sm.GetObjectScene(selectedId) : nullptr;
    
    if (!selectedScene) {
        m_activeFieldIndex = -1;
        m_isEditing = false;
        
        int startY = inspector->getY() + 180;
        r.drawText(inspector->getX() + 10, startY, "No object selected", 0.6f, 0.6f, 0.6f);
        r.drawText(inspector->getX() + 10, startY + 25, "Click on an object", 0.5f, 0.5f, 0.5f);
        r.drawText(inspector->getX() + 10, startY + 50, "in the Hierarchy panel", 0.5f, 0.5f, 0.5f);
        return;
    }
    
    if (!m_isEditing) {
        updateFieldValuesFromObject();
    }
    
    int startY = inspector->getY() + 180;
    int lineHeight = 28;
    int fieldStartX = inspector->getX() + 55;
    int fieldWidth = 65;
    
    // ID объекта
    char idBuf[64];
    sprintf_s(idBuf, "ID: %d", selectedScene->id);
    r.drawText(inspector->getX() + 10, startY - 45, idBuf, 0.6f, 0.6f, 0.8f);
    
    // Имя объекта
    r.drawText(inspector->getX() + 10, startY - 30, "Object: " + selectedScene->name, 0.9f, 0.9f, 0.5f);
    
    // Тип объекта
    std::string typeStr = "Type: ";
    typeStr += selectedScene->pathOfModel.empty() ? "Empty" : "Model";
    r.drawText(inspector->getX() + 10, startY - 15, typeStr, 0.7f, 0.7f, 0.9f);
    
    // Позиция
    r.drawText(inspector->getX() + 10, startY, "Pos:", 0.8f, 0.8f, 0.8f);
    drawInputField(r, m_fieldValues[0], fieldStartX, startY, fieldWidth, 22, m_activeFieldIndex == 0 && m_isEditing);
    drawInputField(r, m_fieldValues[1], fieldStartX + fieldWidth + 5, startY, fieldWidth, 22, m_activeFieldIndex == 1 && m_isEditing);
    drawInputField(r, m_fieldValues[2], fieldStartX + (fieldWidth + 5) * 2, startY, fieldWidth, 22, m_activeFieldIndex == 2 && m_isEditing);
    
    // Вращение
    r.drawText(inspector->getX() + 10, startY + lineHeight, "Rot:", 0.8f, 0.8f, 0.8f);
    drawInputField(r, m_fieldValues[3], fieldStartX, startY + lineHeight, fieldWidth, 22, m_activeFieldIndex == 3 && m_isEditing);
    drawInputField(r, m_fieldValues[4], fieldStartX + fieldWidth + 5, startY + lineHeight, fieldWidth, 22, m_activeFieldIndex == 4 && m_isEditing);
    drawInputField(r, m_fieldValues[5], fieldStartX + (fieldWidth + 5) * 2, startY + lineHeight, fieldWidth, 22, m_activeFieldIndex == 5 && m_isEditing);
    
    // Масштаб
    r.drawText(inspector->getX() + 10, startY + lineHeight * 2, "Scl:", 0.8f, 0.8f, 0.8f);
    drawInputField(r, m_fieldValues[6], fieldStartX, startY + lineHeight * 2, fieldWidth, 22, m_activeFieldIndex == 6 && m_isEditing);
    drawInputField(r, m_fieldValues[7], fieldStartX + fieldWidth + 5, startY + lineHeight * 2, fieldWidth, 22, m_activeFieldIndex == 7 && m_isEditing);
    drawInputField(r, m_fieldValues[8], fieldStartX + (fieldWidth + 5) * 2, startY + lineHeight * 2, fieldWidth, 22, m_activeFieldIndex == 8 && m_isEditing);
}

void ObjectUI::drawInputField(RenderUI& r, const std::string& value, int x, int y, int w, int h, bool isActive) {
    if (isActive) {
        r.drawQuad(x, y, x + w, y + h, 0.25f, 0.3f, 0.4f);
        r.drawQuad(x, y, x + w, y + 1, 0.6f, 0.7f, 1.0f);
    } else {
        r.drawQuad(x, y, x + w, y + h, 0.15f, 0.15f, 0.2f);
        r.drawQuad(x, y, x + w, y + 1, 0.3f, 0.3f, 0.35f);
    }
    
    r.drawQuad(x, y, x + w, y + 1, 0.4f, 0.4f, 0.45f);
    r.drawQuad(x, y + h - 1, x + w, y + h, 0.4f, 0.4f, 0.45f);
    r.drawQuad(x, y, x + 1, y + h, 0.4f, 0.4f, 0.45f);
    r.drawQuad(x + w - 1, y, x + w, y + h, 0.4f, 0.4f, 0.45f);
    
    std::string displayText = value;
    if (displayText.length() > 10) displayText = displayText.substr(0, 8) + "..";
    if (isActive) displayText += "_";
    
    r.drawText(x + 5, y + 5, displayText, 1.0f, 1.0f, 1.0f);
}

void ObjectUI::handleClick(int x, int y, int w, int h, PanelManager& panels) {
    Panel* inspector = panels.getPanel("Inspector");
    if (!inspector || !inspector->visible || inspector->collapsed) {
        m_isEditing = false;
        m_activeFieldIndex = -1;
        return;
    }
    
    auto& sm = SceneManager::Instance();
    int selectedId = sm.GetSelectedObjectScene();
    ObjectScene* selectedScene = (selectedId != 0) ? sm.GetObjectScene(selectedId) : nullptr;
    
    if (!selectedScene) {
        m_isEditing = false;
        m_activeFieldIndex = -1;
        return;
    }
    
    int startY = inspector->getY() + 180;
    int lineHeight = 28;
    int fieldStartX = inspector->getX() + 55;
    int fieldWidth = 65;
    
    if (m_isEditing && m_activeFieldIndex != -1) {
        applyFieldValue();
    }
    
    m_isEditing = false;
    m_activeFieldIndex = -1;
    
    for (int i = 0; i < 9; i++) {
        int fieldX = fieldStartX + (i % 3) * (fieldWidth + 5);
        int fieldY = startY + (i / 3) * lineHeight;
        
        if (x >= fieldX && x <= fieldX + fieldWidth && y >= fieldY && y <= fieldY + 22) {
            m_activeFieldIndex = i;
            m_isEditing = true;
            m_fieldOriginalValues[i] = m_fieldValues[i];
            std::cout << "[ObjectUI] Editing field " << i << " for object: " << selectedScene->name << " (ID: " << selectedScene->id << ")" << std::endl;
            return;
        }
    }
}

void ObjectUI::handleKeyboardInput(WPARAM wParam, char c) {
    if (!m_isEditing || m_activeFieldIndex < 0) return;
    
    std::string& value = m_fieldValues[m_activeFieldIndex];
    
    if (wParam == VK_RETURN) {
        applyFieldValue();
        m_isEditing = false;
        m_activeFieldIndex = -1;
        std::cout << "[ObjectUI] Applied value" << std::endl;
    }
    else if (wParam == VK_ESCAPE) {
        value = m_fieldOriginalValues[m_activeFieldIndex];
        m_isEditing = false;
        m_activeFieldIndex = -1;
        std::cout << "[ObjectUI] Cancelled" << std::endl;
    }
    else if (wParam == VK_BACK) {
        if (!value.empty()) value.pop_back();
    }
    else if (c >= '0' && c <= '9' || c == '.' || c == '-') {
        value += c;
    }
}

void ObjectUI::applyFieldValue() {
    if (m_activeFieldIndex < 0) return;
    
    auto& sm = SceneManager::Instance();
    int selectedId = sm.GetSelectedObjectScene();
    ObjectScene* selectedScene = (selectedId != 0) ? sm.GetObjectScene(selectedId) : nullptr;
    
    if (!selectedScene) return;
    
    float val = 0.0f;
    try {
        val = std::stof(m_fieldValues[m_activeFieldIndex]);
    } catch (...) {
        m_fieldValues[m_activeFieldIndex] = m_fieldOriginalValues[m_activeFieldIndex];
        std::cout << "[ObjectUI] Failed to parse value" << std::endl;
        return;
    }
    
    std::cout << "[ObjectUI] Applying field " << m_activeFieldIndex << " = " << val 
              << " to object: " << selectedScene->name << " (ID: " << selectedScene->id << ")" << std::endl;
    
    glm::vec3 currentPos = selectedScene->getPosition();
    glm::vec3 currentRot = selectedScene->getRotation();
    glm::vec3 currentScale = selectedScene->getScale();
    
    switch (m_activeFieldIndex) {
        case 0: // Pos X
            selectedScene->setPosition(glm::vec3(val, currentPos.y, currentPos.z));
            break;
        case 1: // Pos Y
            selectedScene->setPosition(glm::vec3(currentPos.x, val, currentPos.z));
            break;
        case 2: // Pos Z
            selectedScene->setPosition(glm::vec3(currentPos.x, currentPos.y, val));
            break;
        case 3: // Rot X
            selectedScene->setRotation(glm::vec3(val, currentRot.y, currentRot.z));
            break;
        case 4: // Rot Y
            selectedScene->setRotation(glm::vec3(currentRot.x, val, currentRot.z));
            break;
        case 5: // Rot Z
            selectedScene->setRotation(glm::vec3(currentRot.x, currentRot.y, val));
            break;
        case 6: // Scale X
            selectedScene->setScale(glm::vec3(val, currentScale.y, currentScale.z));
            std::cout << "[ObjectUI] Scale X changed to: " << val << std::endl;
            break;
        case 7: // Scale Y
            selectedScene->setScale(glm::vec3(currentScale.x, val, currentScale.z));
            break;
        case 8: // Scale Z
            selectedScene->setScale(glm::vec3(currentScale.x, currentScale.y, val));
            break;
    }
    
    selectedScene->updateWorldMatrix();
    
    std::cout << "[ObjectUI] After change - New position: " 
              << selectedScene->getPosition().x << ", "
              << selectedScene->getPosition().y << ", "
              << selectedScene->getPosition().z << std::endl;
    std::cout << "[ObjectUI] After change - New scale: " 
              << selectedScene->scaleX << ", "
              << selectedScene->scaleY << ", "
              << selectedScene->scaleZ << std::endl;
    
    updateFieldValuesFromObject();
}