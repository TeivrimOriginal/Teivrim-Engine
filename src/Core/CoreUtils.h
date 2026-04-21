#ifndef CORE_UTILS_H
#define CORE_UTILS_H

#include <string>
#include <windows.h>

namespace CoreUtils {
    // Структура для хранения размеров окна
    struct WindowDimensions {
        int width;
        int height;
        
        WindowDimensions() : width(1280), height(720) {}
        WindowDimensions(int w, int h) : width(w), height(h) {}
        
        void validate() {
            if (width <= 0) width = 1280;
            if (height <= 0) height = 720;
        }
    };

    // Получить размеры клиентской области окна
    WindowDimensions GetClientAreaDimensions(HWND hwnd);
    
    // Открыть файловый диалог для 3D моделей
    bool OpenModelFileDialog(HWND hwnd, std::string& outPath);
    
    // Открыть файловый диалог для проектов
    bool OpenProjectFileDialog(HWND hwnd, std::string& outPath);
    
    // Получить позицию курсора в координатах клиента окна
    POINT GetClientMousePosition(HWND hwnd);
    
    // Установить текст окна с FPS
    void SetWindowTitleWithFPS(HWND hwnd, const char* apiName, int fps);
}

#endif
