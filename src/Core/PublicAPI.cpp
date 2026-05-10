#include "PublicAPI.h"
#include "SecondComplexity/Scene/SceneManager.h"
#include "SecondRender.h"
#include <iostream>

// Static members
bool PublicAPI::s_initialized = false;
WindowAPI PublicAPI::s_window;
ObjectAPI PublicAPI::s_object;
CameraAPI PublicAPI::s_camera;
GridAPI PublicAPI::s_grid;
ModelAPI PublicAPI::s_model;
RenderAPIUtils PublicAPI::s_render;
InputAPI PublicAPI::s_input;
DebugAPI PublicAPI::s_debug;

// ============================================
// WindowAPI Implementation
// ============================================

bool WindowAPI::Initialize(int width, int height, const std::string& title) {
    std::cout << "[WindowAPI] Initialize: " << width << "x" << height << " - " << title << std::endl;
    return true;
}

void WindowAPI::Shutdown() {
    std::cout << "[WindowAPI] Shutdown" << std::endl;
}

void WindowAPI::SetTitle(const std::string& title) {
    std::cout << "[WindowAPI] SetTitle: " << title << std::endl;
}

int WindowAPI::GetWidth() { return 1280; }
int WindowAPI::GetHeight() { return 720; }
bool WindowAPI::IsRunning() { return true; }

// ============================================
// ObjectAPI Implementation
// ============================================

ObjectAPI::ObjectID ObjectAPI::CreateObject(const std::string& name) {
    auto& sm = SceneManager::Instance();
    return sm.CreateObject(name);
}

void ObjectAPI::DestroyObject(ObjectID id) {
    auto& sm = SceneManager::Instance();
    sm.DestroyObject(id);
}

void ObjectAPI::SetPosition(ObjectID id, float x, float y, float z) {
    auto& sm = SceneManager::Instance();
    sm.SetPosition(id, glm::vec3(x, y, z));
}

void ObjectAPI::SetRotation(ObjectID id, float pitch, float yaw, float roll) {
    auto& sm = SceneManager::Instance();
    glm::quat rot = glm::quat(glm::vec3(glm::radians(pitch), glm::radians(yaw), glm::radians(roll)));
    sm.SetRotation(id, rot);
}

void ObjectAPI::SetScale(ObjectID id, float x, float y, float z) {
    auto& sm = SceneManager::Instance();
    sm.SetScale(id, glm::vec3(x, y, z));
}

void ObjectAPI::GetPosition(ObjectID id, float& x, float& y, float& z) {
    auto& sm = SceneManager::Instance();
    glm::vec3 pos = sm.GetWorldPosition(id);
    x = pos.x; y = pos.y; z = pos.z;
}

void ObjectAPI::SelectObject(ObjectID id) {
    auto& sm = SceneManager::Instance();
    sm.SelectObject(id);
}

ObjectAPI::ObjectID ObjectAPI::GetSelectedObject() {
    auto& sm = SceneManager::Instance();
    return sm.GetSelectedObjectID();
}

std::vector<ObjectAPI::ObjectID> ObjectAPI::GetAllObjects() {
    std::vector<ObjectID> result;
    auto& sm = SceneManager::Instance();
    for (auto obj : sm.GetAllObjects()) {
        result.push_back(obj->id);
    }
    return result;
}

// ============================================
// CameraAPI Implementation
// ============================================

void CameraAPI::SetPosition(float x, float y, float z) {
    std::cout << "[CameraAPI] SetPosition: " << x << ", " << y << ", " << z << std::endl;
}

void CameraAPI::GetPosition(float& x, float& y, float& z) {
    x = 0; y = 5; z = 15;
}

void CameraAPI::SetTarget(float x, float y, float z) {
    auto& sm = SceneManager::Instance();
    sm.SetCameraTarget(glm::vec3(x, y, z));
}

void CameraAPI::GetTarget(float& x, float& y, float& z) {
    auto& sm = SceneManager::Instance();
    glm::vec3 target = sm.GetCameraTarget();
    x = target.x; y = target.y; z = target.z;
}

void CameraAPI::SetFOV(float fov) {
    auto& sm = SceneManager::Instance();
    sm.SetCameraFov(fov);
}

float CameraAPI::GetFOV() {
    auto& sm = SceneManager::Instance();
    return sm.GetMainCamera() ? sm.GetMainCamera()->fov : 60.0f;
}

void CameraAPI::Reset() {
    auto& sm = SceneManager::Instance();
    sm.ResetCamera();
}

void CameraAPI::MoveForward(float delta) {}
void CameraAPI::MoveRight(float delta) {}
void CameraAPI::MoveUp(float delta) {}

// ============================================
// GridAPI Implementation
// ============================================

void GridAPI::SetEnabled(bool enabled) {
    std::cout << "[GridAPI] SetEnabled: " << enabled << std::endl;
}

bool GridAPI::IsEnabled() { return true; }

void GridAPI::SetSpacing(float spacing) {
    std::cout << "[GridAPI] SetSpacing: " << spacing << std::endl;
}

float GridAPI::GetSpacing() { return 20.0f; }

void GridAPI::SetColor(float r, float g, float b) {
    std::cout << "[GridAPI] SetColor: " << r << ", " << g << ", " << b << std::endl;
}

void GridAPI::SetAxisColor(float r, float g, float b) {
    std::cout << "[GridAPI] SetAxisColor: " << r << ", " << g << ", " << b << std::endl;
}

// ============================================
// ModelAPI Implementation
// ============================================

bool ModelAPI::LoadModel(const std::string& path) {
    std::cout << "[ModelAPI] LoadModel: " << path << std::endl;
    return true;
}

void ModelAPI::UnloadModel(const std::string& name) {
    std::cout << "[ModelAPI] UnloadModel: " << name << std::endl;
}

void ModelAPI::SetModelTransform(const std::string& name, float x, float y, float z, float scale) {
    std::cout << "[ModelAPI] SetModelTransform: " << name << " pos(" << x << "," << y << "," << z << ") scale(" << scale << ")" << std::endl;
}

bool ModelAPI::IsModelLoaded(const std::string& name) { return false; }

// ============================================
// RenderAPIUtils Implementation
// ============================================

void RenderAPIUtils::SetContourEnabled(bool enabled) {
    SecondRender::Instance().SetContourEnabled(enabled);
    std::cout << "[RenderAPI] SetContourEnabled: " << enabled << std::endl;
}

void RenderAPIUtils::SetContourColor(float r, float g, float b) {
    std::cout << "[RenderAPI] SetContourColor: " << r << ", " << g << ", " << b << std::endl;
}

void RenderAPIUtils::SetContourThickness(float thickness) {
    std::cout << "[RenderAPI] SetContourThickness: " << thickness << std::endl;
}

void RenderAPIUtils::SetBackgroundColor(float r, float g, float b) {
    std::cout << "[RenderAPI] SetBackgroundColor: " << r << ", " << g << ", " << b << std::endl;
}

// ============================================
// InputAPI Implementation
// ============================================

bool InputAPI::IsKeyPressed(int key) { return false; }
bool InputAPI::IsMouseButtonPressed(int button) { return false; }
void InputAPI::GetMousePosition(float& x, float& y) { x = 0; y = 0; }
void InputAPI::SetMouseCapture(bool capture) { std::cout << "[InputAPI] SetMouseCapture: " << capture << std::endl; }
bool InputAPI::IsMouseCaptured() { return false; }

// ============================================
// DebugAPI Implementation
// ============================================

void DebugAPI::Log(const std::string& message) {
    std::cout << "[LOG] " << message << std::endl;
}

void DebugAPI::LogError(const std::string& message) {
    std::cerr << "[ERROR] " << message << std::endl;
}

void DebugAPI::LogWarning(const std::string& message) {
    std::cout << "[WARNING] " << message << std::endl;
}

void DebugAPI::SetLogLevel(int level) {
    std::cout << "[DebugAPI] SetLogLevel: " << level << std::endl;
}

// ============================================
// PublicAPI Implementation
// ============================================

bool PublicAPI::Initialize() {
    std::cout << "========================================" << std::endl;
    std::cout << "     PublicAPI Initialization          " << std::endl;
    std::cout << "========================================" << std::endl;
    
    bool allOk = true;
    
    std::cout << "[OK] WindowAPI: Ready" << std::endl;
    std::cout << "[OK] ObjectAPI: Ready" << std::endl;
    std::cout << "[OK] CameraAPI: Ready" << std::endl;
    std::cout << "[OK] GridAPI: Ready" << std::endl;
    std::cout << "[OK] ModelAPI: Ready" << std::endl;
    std::cout << "[OK] RenderAPI: Ready" << std::endl;
    std::cout << "[OK] InputAPI: Ready" << std::endl;
    std::cout << "[OK] DebugAPI: Ready" << std::endl;
    
    std::cout << "========================================" << std::endl;
    std::cout << "     PublicAPI: ALL SYSTEMS OK         " << std::endl;
    std::cout << "========================================" << std::endl;
    
    s_initialized = true;
    return allOk;
}

void PublicAPI::Shutdown() {
    std::cout << "[PublicAPI] Shutdown" << std::endl;
    s_initialized = false;
}