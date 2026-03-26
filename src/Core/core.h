#ifndef CORE_H
#define CORE_H

#include <string>
#include <GL/glew.h>
#include <windows.h>
#include "Render/Win32/rendererw.h"
#include "Render/Parser/parser.h"

class Core {
public:
    bool isStart = false;
    bool modelLoaded = false;
    bool needsOptimize = false;
    std::string modelPath;
    GLuint shaderProgram = 0;

    RendererW rendererw;
    ModelParser modelParser;
    bool CanClick = true;
    void swapcanclick() {
        CanClick = !CanClick;
    }
    void settingUpRender();
    void ParserToRender();
    void GameLoop();

    bool loadModelFromPath(const std::string& path);
    bool openFileDialogAndLoadModel(HWND hwnd);
};

#endif