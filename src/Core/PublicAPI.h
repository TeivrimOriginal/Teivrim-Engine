// PublicAPI.h - Фундамент для скриптовой системы
#ifndef PUBLIC_API_H
#define PUBLIC_API_H

#include <iostream>
#include <string>
#include <functional>
#include <vector>
#include <cstdint>

// ============================================
// РАЗДЕЛ: Управление окном и рендером
// ============================================

class WindowAPI {
public:
    static bool Initialize(int width, int height, const std::string& title);
    static void Shutdown();
    static void SetTitle(const std::string& title);
    static int GetWidth();
    static int GetHeight();
    static bool IsRunning();
};

// ============================================
// РАЗДЕЛ: Управление 3D объектами
// ============================================

class ObjectAPI {
public:
    using ObjectID = uint32_t;
    
    static ObjectID CreateObject(const std::string& name);
    static void DestroyObject(ObjectID id);
    static void SetPosition(ObjectID id, float x, float y, float z);
    static void SetRotation(ObjectID id, float pitch, float yaw, float roll);
    static void SetScale(ObjectID id, float x, float y, float z);
    static void GetPosition(ObjectID id, float& x, float& y, float& z);
    static void SelectObject(ObjectID id);
    static ObjectID GetSelectedObject();
    static std::vector<ObjectID> GetAllObjects();
};

// ============================================
// РАЗДЕЛ: Управление камерой
// ============================================

class CameraAPI {
public:
    static void SetPosition(float x, float y, float z);
    static void GetPosition(float& x, float& y, float& z);
    static void SetTarget(float x, float y, float z);
    static void GetTarget(float& x, float& y, float& z);
    static void SetFOV(float fov);
    static float GetFOV();
    static void Reset();
    static void MoveForward(float delta);
    static void MoveRight(float delta);
    static void MoveUp(float delta);
};

// ============================================
// РАЗДЕЛ: Управление сеткой (Grid)
// ============================================

class GridAPI {
public:
    static void SetEnabled(bool enabled);
    static bool IsEnabled();
    static void SetSpacing(float spacing);
    static float GetSpacing();
    static void SetColor(float r, float g, float b);
    static void SetAxisColor(float r, float g, float b);
};

// ============================================
// РАЗДЕЛ: Управление моделями
// ============================================

class ModelAPI {
public:
    static bool LoadModel(const std::string& path);
    static void UnloadModel(const std::string& name);
    static void SetModelTransform(const std::string& name, float x, float y, float z, float scale);
    static bool IsModelLoaded(const std::string& name);
};

// ============================================
// РАЗДЕЛ: Управление рендером (обводка, материалы)
// ============================================

class RenderAPIUtils {
public:
    static void SetContourEnabled(bool enabled);
    static void SetContourColor(float r, float g, float b);
    static void SetContourThickness(float thickness);
    static void SetBackgroundColor(float r, float g, float b);
};

// ============================================
// РАЗДЕЛ: Ввод с клавиатуры и мыши
// ============================================

class InputAPI {
public:
    static bool IsKeyPressed(int key);
    static bool IsMouseButtonPressed(int button);
    static void GetMousePosition(float& x, float& y);
    static void SetMouseCapture(bool capture);
    static bool IsMouseCaptured();
};

// ============================================
// РАЗДЕЛ: Логирование и отладка
// ============================================

class DebugAPI {
public:
    static void Log(const std::string& message);
    static void LogError(const std::string& message);
    static void LogWarning(const std::string& message);
    static void SetLogLevel(int level);
};

// ============================================
// ГЛАВНЫЙ API - ТОЧКА ВХОДА
// ============================================

class PublicAPI {
public:
    static bool Initialize();
    static void Shutdown();
    static bool IsInitialized() { return s_initialized; }
    
    // Доступ к разделам API
    static WindowAPI& Window() { return s_window; }
    static ObjectAPI& Object() { return s_object; }
    static CameraAPI& Camera() { return s_camera; }
    static GridAPI& Grid() { return s_grid; }
    static ModelAPI& Model() { return s_model; }
    static RenderAPIUtils& Render() { return s_render; }
    static InputAPI& Input() { return s_input; }
    static DebugAPI& Debug() { return s_debug; }
    
private:
    static bool s_initialized;
    static WindowAPI s_window;
    static ObjectAPI s_object;
    static CameraAPI s_camera;
    static GridAPI s_grid;
    static ModelAPI s_model;
    static RenderAPIUtils s_render;
    static InputAPI s_input;
    static DebugAPI s_debug;
};

#endif