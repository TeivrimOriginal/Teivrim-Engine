#ifndef RENDERERV_H
#define RENDERERV_H

#include <glm/glm.hpp>
#include "../../../Control/camera.h"
#include "../../../Application/WindowAPIsupport/Win32/InitialWin32.h"
#include "../Parser/parser.h"

class RendererV {
public:
    RendererV();
    ~RendererV();
    
    bool initialize(InitialWin32* win);
    void cleanup();
    void renderModel(Camera& camera);
    void setAnimateModel(bool animate);
    
private:
    InitialWin32* window;
    bool animateModel;
    bool initialized;
};

#endif