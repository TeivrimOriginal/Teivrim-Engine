// ObjectUI.cpp
#include "ObjectUI.h"
#include "../Core/SecondComplexity/Scene/SceneManager.h"
#include "../Core/Vulkan.h"
#include <iostream>

ObjectUI::ObjectUI() 
    : m_activeFieldIndex(-1)
    , m_isEditing(false)
    , m_vulkan(nullptr)
{
    // Инициализируем поля ввода (9 полей: Pos X,Y,Z | Rot X,Y,Z | Scale X,Y,Z)
    for (int i = 0; i < 9; i++) {
        m_fieldValues.push_back("0");
        m_fieldOriginalValues.push_back("0");
    }
}

ObjectUI::~ObjectUI() {}

void ObjectUI::updateFieldValuesFromObject() {
    auto& sm = SceneManager::Instance();
    SceneObject* selected = sm.GetSelectedObject();
    if (!selected) return;
    
    // Получаем МИРОВУЮ позицию
    glm::vec3 worldPos = sm.GetWorldPosition(selected->id);
    glm::vec3 worldRot = selected->localTransform.getEulerAngles();
    glm::vec3 worldScale = selected->localTransform.scale;
    
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
    SceneObject* selected = sm.GetSelectedObject();
    
    if (!selected) {
        m_activeFieldIndex = -1;
        m_isEditing = false;
        return;
    }
    
    // Если не редактируем - обновляем значения из объекта
    if (!m_isEditing) {
        updateFieldValuesFromObject();
    }
    
    int startY = inspector->getY() + 180;
    int lineHeight = 28;
    int fieldStartX = inspector->getX() + 55;
    int fieldWidth = 65;
    
    // Имя и тип объекта
    r.drawText(inspector->getX() + 10, startY - 30, "Object: " + selected->name, 0.9f, 0.9f, 0.5f);
    
    std::string typeStr = "Type: ";
    switch (selected->type) {
        case ObjectType::CAMERA: typeStr += "Camera"; break;
        case ObjectType::MODEL: typeStr += "Model"; break;
        case ObjectType::LIGHT: typeStr += "Light"; break;
        default: typeStr += "Empty";
    }
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
    // Фон
    if (isActive) {
        r.drawQuad(x, y, x + w, y + h, 0.25f, 0.3f, 0.4f);
        r.drawQuad(x, y, x + w, y + 1, 0.6f, 0.7f, 1.0f);
    } else {
        r.drawQuad(x, y, x + w, y + h, 0.15f, 0.15f, 0.2f);
        r.drawQuad(x, y, x + w, y + 1, 0.3f, 0.3f, 0.35f);
    }
    
    // Рамка
    r.drawQuad(x, y, x + w, y + 1, 0.4f, 0.4f, 0.45f);
    r.drawQuad(x, y + h - 1, x + w, y + h, 0.4f, 0.4f, 0.45f);
    r.drawQuad(x, y, x + 1, y + h, 0.4f, 0.4f, 0.45f);
    r.drawQuad(x + w - 1, y, x + w, y + h, 0.4f, 0.4f, 0.45f);
    
    // Текст
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
    
    int startY = inspector->getY() + 180;
    int lineHeight = 28;
    int fieldStartX = inspector->getX() + 55;
    int fieldWidth = 65;
    
    // Если было активное поле - применяем значение
    if (m_isEditing && m_activeFieldIndex != -1) {
        applyFieldValue();
    }
    
    m_isEditing = false;
    m_activeFieldIndex = -1;
    
    // Проверяем клик по полям
    for (int i = 0; i < 9; i++) {
        int fieldX = fieldStartX + (i % 3) * (fieldWidth + 5);
        int fieldY = startY + (i / 3) * lineHeight;
        
        if (x >= fieldX && x <= fieldX + fieldWidth && y >= fieldY && y <= fieldY + 22) {
            m_activeFieldIndex = i;
            m_isEditing = true;
            m_fieldOriginalValues[i] = m_fieldValues[i];
            std::cout << "[ObjectUI] Editing field " << i << std::endl;
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
    SceneObject* selected = sm.GetSelectedObject();
    if (!selected) return;
    
    float val = 0.0f;
    try {
        val = std::stof(m_fieldValues[m_activeFieldIndex]);
    } catch (...) {
        m_fieldValues[m_activeFieldIndex] = m_fieldOriginalValues[m_activeFieldIndex];
        std::cout << "[ObjectUI] Failed to parse value" << std::endl;
        return;
    }
    
    std::cout << "[ObjectUI] Applying field " << m_activeFieldIndex << " = " << val << std::endl;
    
    // Получаем текущую мировую позицию для позиционных полей
    glm::vec3 currentWorldPos = sm.GetWorldPosition(selected->id);
    
    switch (m_activeFieldIndex) {
        case 0: // Pos X
            {
                glm::vec3 newPos = currentWorldPos;
                newPos.x = val;
                // Перемещаем объект так, чтобы его мировая позиция стала val
                selected->localTransform.position.x += (newPos.x - currentWorldPos.x);
            }
            break;
        case 1: // Pos Y
            {
                glm::vec3 newPos = currentWorldPos;
                newPos.y = val;
                selected->localTransform.position.y += (newPos.y - currentWorldPos.y);
            }
            break;
        case 2: // Pos Z
            {
                glm::vec3 newPos = currentWorldPos;
                newPos.z = val;
                selected->localTransform.position.z += (newPos.z - currentWorldPos.z);
            }
            break;
        case 3: // Rot X
            {
                glm::vec3 euler = selected->localTransform.getEulerAngles();
                euler.x = val;
                selected->localTransform.setEulerAngles(euler.x, euler.y, euler.z);
            }
            break;
        case 4: // Rot Y
            {
                glm::vec3 euler = selected->localTransform.getEulerAngles();
                euler.y = val;
                selected->localTransform.setEulerAngles(euler.x, euler.y, euler.z);
            }
            break;
        case 5: // Rot Z
            {
                glm::vec3 euler = selected->localTransform.getEulerAngles();
                euler.z = val;
                selected->localTransform.setEulerAngles(euler.x, euler.y, euler.z);
            }
            break;
        case 6: // Scale X
            selected->localTransform.scale.x = val;
            break;
        case 7: // Scale Y
            selected->localTransform.scale.y = val;
            break;
        case 8: // Scale Z
            selected->localTransform.scale.z = val;
            break;
    }
    
    selected->markDirty();
    sm.UpdateWorldTransforms();
    
    // Обновляем Vulkan трансформацию
    if (m_vulkan) {
        m_vulkan->setModelTransform(selected->name, sm.GetWorldMatrix(selected->id));
    }
    
    // Обновляем отображение значений
    updateFieldValuesFromObject();
    
    std::cout << "[ObjectUI] New world position: " 
              << sm.GetWorldPosition(selected->id).x << ", "
              << sm.GetWorldPosition(selected->id).y << ", "
              << sm.GetWorldPosition(selected->id).z << std::endl;
}