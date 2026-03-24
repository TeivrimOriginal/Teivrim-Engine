cmake_minimum_required(VERSION 3.10)
project(PolygonEditor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O3")

include_directories(include)
link_directories(${CMAKE_SOURCE_DIR}/lib)

# ЯВНО указываем нужные файлы, а не все подряд
set(SOURCES
    src/main.cpp
    src/Interface/Panels.cpp
    src/Interface/ObjectUI.cpp
    src/Interface/InterfaceManager.cpp
    src/Control/Input.cpp
    src/Control/camera.cpp
    src/Application/application.cpp
    src/Application/WindowAPIsupport/Win32/InitialWin32.cpp
    src/Core/core.cpp
    src/Core/Render/Win32/RenderUI.cpp
    src/Core/Render/Win32/rendererw.cpp
    src/Core/Render/Parser/parser.cpp
    src/Core/Render/Win32/stb_truetype_impl.cpp
    src/Core/Render/Win32/GraphicsApiSupport/RenderUI_OpenGL.cpp
)

# НЕ включаем тестовые файлы!
# src/TEST/TestTextureWithFBXparser.cpp - исключен
# src/TEST/interfaceplus3d.cpp - исключен

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME}
    assimp
    glfw3
    glew32
    opengl32
    gdi32
    ws2_32
    comdlg32
)