#include "InterfaceManager.h"
#include <iostream>

InterfaceManager::InterfaceManager(Core* corePtr) : window(nullptr), core(corePtr) {
    objectUI.createButton("Button 1", 10, 10, 80, 25, []() {
        std::cout << "Button 1 clicked!" << std::endl;
    });
    
    objectUI.createButton("Button 2", 100, 10, 80, 25, [this]() {
        std::cout << "Button 2 clicked!" << std::endl;
        if (core) {
            SwapFlag(*core);
            std::cout << "Core.isStart = " << core->isStart << std::endl;
        }
    });
    
    objectUI.createButton("Left Button", 20, 50, 100, 30, []() {
        std::cout << "Left panel button clicked!" << std::endl;
    });
    
    objectUI.createButton("Right Button", 20, 100, 100, 30, []() {
        std::cout << "Right panel button clicked!" << std::endl;
    });
    
    objectUI.createButton("Load Model", 20, 5, 160, 30, [this]() {
        std::cout << "Load Model clicked" << std::endl;
        if (core) {
            HWND hwnd = getHWND();
            if (core->openFileDialogAndLoadModel(hwnd)) {
                std::cout << "Model successfully loaded" << std::endl;
            } else {
                std::cout << "Model not loaded or cancelled" << std::endl;
            }
        }
    });
    
    objectUI.attachToPanel("Button 1", PanelType::Top);
    objectUI.attachToPanel("Button 2", PanelType::Top);
    objectUI.attachToPanel("Left Button", PanelType::Left);
    objectUI.attachToPanel("Right Button", PanelType::Right);
    objectUI.attachToPanel("Load Model", PanelType::Bottom);
    updatePanelMinSizes();
}

Dimensions InterfaceManager::getDimensions() {
    Dimensions dims = {0, 0};
    if (!window) return dims;
    
    HWND hwnd = window->getHWND();
    if (!hwnd) return dims;
    
    RECT rect;
    GetClientRect(hwnd, &rect);
    dims.width = rect.right - rect.left;
    dims.height = rect.bottom - rect.top;
    
    return dims;
}

void InterfaceManager::clearScreen(int width, int height) {
    glViewport(0, 0, width, height);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
void InterfaceManager::BlockMoveToMainWindow(int x, int y) {
    bool isClick = CheckerClickToPanel( x, y);
    if (isClick && isStopMove == false){
        swapclick();
        std::cout << "CLICK: Panel " << std::endl;
            
        isStopMove = true;
    } else if (isStopMove && !isClick){
        swapclick();
                    
        std::cout << "CLICK: Render " << std::endl;

        
        isStopMove = false;
    }
}
bool InterfaceManager::CheckerClickToPanel(int x, int y) {
    Dimensions dims = getDimensions();
    if (dims.width == 0 || dims.height == 0) return false;
    
    std::vector<PanelType> panelsToCheck = {
        PanelType::Left,
        PanelType::Right,
        PanelType::Top,
        PanelType::Bottom
    };
    
    for (PanelType panel : panelsToCheck) {
        int outX, outY, outW, outH;
        panels.getPanelBounds(panel, dims.width, dims.height, outX, outY, outW, outH);
        
        if (x >= outX && x <= outX + outW && y >= outY && y <= outY + outH) {
            std::cout << "Clicked on panel: " << static_cast<int>(panel) << std::endl;
            return true;
        }
    }
    
    return false;
}

void InterfaceManager::setup3DViewport(const Dimensions& dims) {
    PanelDimensions panelDims = panels.getDimensions(dims.width, dims.height);
    
    glViewport(panelDims.leftPanelWidth, panelDims.topPanelHeight,
               dims.width - (panelDims.leftPanelWidth + panelDims.rightPanelWidth),
               dims.height - (panelDims.topPanelHeight + panelDims.bottomPanelHeight));
    glEnable(GL_DEPTH_TEST);
}

void InterfaceManager::renderStatic() {
    Dimensions dims = getDimensions();
    if (dims.width == 0 || dims.height == 0) return;
    
    GLint program;
    GLint viewport[4];
    GLboolean depthTest;
    renderer.saveState(program, viewport, depthTest);
    
    renderer.setup2D(dims.width, dims.height);
    
    glBegin(GL_QUADS);
    panels.render(renderer, dims.width, dims.height);
    objectUI.render(renderer, dims.width, dims.height, panels);
    glEnd();
    
    renderer.drawText(10, dims.height - 25, "3D Viewer v1.0", 1.0f, 1.0f, 1.0f);
    
    if (core && core->modelLoaded) {
        std::string status = "Model: " + core->modelPath.substr(core->modelPath.find_last_of("/\\") + 1);
        renderer.drawText(10, dims.height - 40, status, 0.5f, 0.8f, 0.5f);
    } else {
        renderer.drawText(10, dims.height - 40, "No model loaded", 1.0f, 0.8f, 0.3f);
    }
    
    renderer.drawText(dims.width - 150, dims.height - 25, "Press ESC to exit", 0.7f, 0.7f, 0.7f);
    
    renderer.restoreMatrices();
    renderer.restoreState(program, viewport, depthTest);
}

void InterfaceManager::renderDynamic() {
    Dimensions dims = getDimensions();
    if (dims.width == 0 || dims.height == 0) return;
    
    GLint program;
    GLint viewport[4];
    GLboolean depthTest;
    renderer.saveState(program, viewport, depthTest);
    
    renderer.setup2D(dims.width, dims.height);
    
    renderer.restoreMatrices();
    renderer.restoreState(program, viewport, depthTest);
}

void InterfaceManager::handleClick(int x, int y) {
    Dimensions dims = getDimensions();
    objectUI.handleClick(x, y, dims.width, dims.height, panels);
}

void InterfaceManager::handleMouseDown(int x, int y) {
    Dimensions dims = getDimensions();
    if (dims.width == 0 || dims.height == 0) return;
    
    // Проверяем, нажали ли на грань
    draggingEdge = panels.getEdgeAt(x, y, dims.width, dims.height);
    
    if (draggingEdge != PanelType::None) {
        isDragging = true;
        dragStartX = x;
        dragStartY = y;
        
        // Запоминаем начальный размер панели
        switch (draggingEdge) {
            case PanelType::Left:
                dragStartValue = panels.getLeftWidth();
                break;
            case PanelType::Right:
                dragStartValue = panels.getRightWidth();
                break;
            case PanelType::Top:
                dragStartValue = panels.getTopHeight();
                break;
            case PanelType::Bottom:
                dragStartValue = panels.getBottomHeight();
                break;
            default:
                break;
        }
        
        // Захватываем мышь для отслеживания движения за пределами окна
        if (window) {
            SetCapture(window->getHWND());
        }
    }
}

void InterfaceManager::handleMouseMove(int x, int y) {
    Dimensions dims = getDimensions();
    if (dims.width == 0 || dims.height == 0) return;
    
    // Если перетаскиваем грань
    if (isDragging && draggingEdge != PanelType::None) {
        int delta = 0;
        
        // Вычисляем дельту в зависимости от оси грани
        switch (draggingEdge) {
            case PanelType::Left:  // Вертикальная грань - двигаем по X
                delta = x - dragStartX;
                panels.setLeftWidth(dragStartValue + delta);
                break;
            case PanelType::Right: // Вертикальная грань - двигаем по X
                delta = dragStartX - x;  // Инвертируем для правой грани
                panels.setRightWidth(dragStartValue + delta);
                break;
            case PanelType::Top:   // Горизонтальная грань - двигаем по Y
                delta = y - dragStartY;
                panels.setTopHeight(dragStartValue + delta);
                break;
            case PanelType::Bottom: // Горизонтальная грань - двигаем по Y
                delta = dragStartY - y;  // Инвертируем для нижней грани
                panels.setBottomHeight(dragStartValue + delta);
                break;
            default:
                break;
        }
        
        // Обновляем позиции UI объектов после изменения размеров
        objectUI.updatePositions(dims.width, dims.height, panels);
        return;
    }
    
    // Если не перетаскиваем, проверяем наведение на грани
    PanelType edge = panels.getEdgeAt(x, y, dims.width, dims.height);
    if (edge != PanelType::None) {
        // Меняем курсор
        HCURSOR cursor = getCursorForEdge(edge);
        SetCursor(cursor);
    }
}

void InterfaceManager::handleMouseUp(int x, int y) {
    if (isDragging) {
        isDragging = false;
        draggingEdge = PanelType::None;
        
        // Отпускаем захват мыши
        if (window) {
            ReleaseCapture();
        }
        
        std::cout << "Stopped dragging" << std::endl;
    }
}

HCURSOR InterfaceManager::getCursorForEdge(PanelType edge) const {
    switch (edge) {
        case PanelType::Left:
        case PanelType::Right:
            return LoadCursor(NULL, IDC_SIZEWE);  // Горизонтальная стрелка
        case PanelType::Top:
        case PanelType::Bottom:
            return LoadCursor(NULL, IDC_SIZENS);  // Вертикальная стрелка
        default:
            return LoadCursor(NULL, IDC_ARROW);
    }
}

void InterfaceManager::SwapFlag(Core &A) {
    A.isStart = !A.isStart;
}

HWND InterfaceManager::getHWND() const {
    return window ? window->getHWND() : nullptr;
}

void InterfaceManager::updatePanelMinSizes() {
    int minLeftWidth = objectUI.getMinWidthForPanel(PanelType::Left);
    int minRightWidth = objectUI.getMinWidthForPanel(PanelType::Right);
    int minTopHeight = objectUI.getMinHeightForPanel(PanelType::Top);
    int minBottomHeight = objectUI.getMinHeightForPanel(PanelType::Bottom);
    
    panels.updateMinSizes(minLeftWidth, minRightWidth, minTopHeight, minBottomHeight);
}