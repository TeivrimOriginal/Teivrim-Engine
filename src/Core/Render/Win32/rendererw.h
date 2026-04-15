#ifndef RENDERERW_H
#define RENDERERW_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include "../Parser/parser.h"
#include "../../../Control/camera.h"
#include "../../../Application/WindowAPIsupport/Win32/InitialWin32.h"

struct OpenGLTexture {
    unsigned int id;
    std::string type;
    std::string path;
};

class RendererW {
public:
    RendererW();
    ~RendererW();
    
    bool initialize(InitialWin32* win);
    void cleanup();
    
    void beginFrame();
    void endFrame();
    void renderModel(const ModelParser& model, GLuint shaderProgram, Camera& camera);
    
    void setAnimateModel(bool animate) { animateModel = animate; }
    GLuint createMeshBuffers(const StandardMesh& mesh);
    void renderStandardMesh(const StandardMesh& mesh, GLuint shaderProgram);
    
    InitialWin32* getWindow() const { return window; }
    
    GLuint initShaders();
    void optimize(const ModelParser& model, GLuint shaderProgram);
    
    InitialWin32* window;
    std::vector<GLuint> VAOs;
    std::vector<GLuint> VBOs;
    std::vector<GLuint> EBOs;
    bool animateModel;
    
    static GLuint compileShader(const char* source, GLenum type);
    static const char* vertexShaderSource;
    static const char* fragmentShaderSource;

private:
    struct MeshBuffers {
        GLuint vao = 0;
        GLuint vbo = 0;
        GLuint ebo = 0;
        int indexCount = 0;
        std::vector<OpenGLTexture> textures;
    };
    
    std::vector<MeshBuffers> meshVAOs;
    GLuint cachedShaderProgram = 0;
    GLint cachedModelLoc = -1;
    GLint cachedViewLoc = -1;
    GLint cachedProjectionLoc = -1;
    GLint cachedLightColorLoc = -1;
    GLint cachedLightPosLoc = -1;
    GLint cachedViewPosLoc = -1;
    GLint cachedUseTextureLoc = -1;
    GLint cachedObjectColorLoc = -1;
    
    unsigned int textureFromRawData(const RawTextureData& rawData);
    unsigned int createWhiteTexture();
};

#endif