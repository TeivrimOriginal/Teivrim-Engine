#include "InterfaceManager.h"
#include <iostream>

InterfaceManager::InterfaceManager(Core* corePtr) : window(nullptr), core(corePtr), panelManager(nullptr) {
    panelManager = new PanelManager();
    setupNewPanels();
    
    objectUI.createButton("Load Model", 20, 5, 160, 30, [this]() {
        if (core) {
            HWND hwnd = getHWND();
            core->openFileDialogAndLoadModel(hwnd);
        }
    });
    objectUI.attachToPanel("Load Model", PanelType::Bottom);
}

InterfaceManager::~InterfaceManager() { 
    delete panelManager; 
}

void InterfaceManager::setupNewPanels() {
    if (!panelManager) return;
    
    Dimensions dims = getDimensions();
    int sw = dims.width > 0 ? dims.width : 1280;
    int sh = dims.height > 0 ? dims.height : 720;
    
    int topHeight = 50;
    int bottomHeight = 100;
    int leftWidth = 250;
    int rightWidth = 250;
    
    auto top = panelManager->create("Scene Control", 0, 0, sw, topHeight);
    top->setDockSide(2, sw, sh);
    top->setMinSize(400, 40);
    top->setMaxSize(2000, 80);
    
    auto bottom = panelManager->create("Asset Manager", 0, sh - bottomHeight, sw, bottomHeight);
    bottom->setDockSide(3, sw, sh);
    bottom->setMinSize(300, 80);
    bottom->setMaxSize(2000, 200);
    
    auto left = panelManager->create("Hierarchy", 0, topHeight, leftWidth, sh - topHeight - bottomHeight);
    left->setDockSide(0, sw, sh);
    left->setMinSize(150, 200);
    left->setMaxSize(400, 2000);
    
    auto right = panelManager->create("Inspector", sw - rightWidth, topHeight, rightWidth, sh - topHeight - bottomHeight);
    right->setDockSide(1, sw, sh);
    right->setMinSize(180, 200);
    right->setMaxSize(500, 2000);
    
    auto view3D = panelManager->create("3D Viewport", leftWidth, topHeight, sw - leftWidth - rightWidth, sh - topHeight - bottomHeight, true);
    view3D->setMinSize(300, 200);
    view3D->setMaxSize(2000, 2000);
    
    auto console = panelManager->create("Console", 400, 300, 500, 200, false);
    console->setMinSize(200, 100);
    console->setMaxSize(800, 400);
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

void InterfaceManager::clearScreen(int w, int h) {
    glViewport(0, 0, w, h);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void InterfaceManager::setup3DViewport(const Dimensions& d) {
    int l=0, r=0, t=0, b=0;
    for (auto p : panelManager->getAll()) {
        if (!p->isVisible()) continue;
        int side = p->getDockSide();
        if (side == 0) l = p->getWidth();
        else if (side == 1) r = p->getWidth();
        else if (side == 2) t = p->getHeight();
        else if (side == 3) b = p->getHeight();
    }
    int vx = l, vy = t, vw = d.width - l - r, vh = d.height - t - b;
    if (vw > 0 && vh > 0) glViewport(vx, vy, vw, vh);
    else glViewport(0, 0, d.width, d.height);
    glEnable(GL_DEPTH_TEST);
}

void InterfaceManager::renderStatic() {
    Dimensions d = getDimensions();
    if (d.width == 0 || d.height == 0) return;
    
    GLint prog, vp[4];
    GLboolean dt;
    renderer.saveState(prog, vp, dt);
    renderer.setup2D(d.width, d.height);
    
    for (auto p : panelManager->getAll()) {
        if (p->getDockSide() != -1) p->updateDock(d.width, d.height);
    }
    
    panelManager->render(renderer);
    
    glBegin(GL_QUADS);
    objectUI.render(renderer, d.width, d.height, panels);
    glEnd();
    
    renderer.drawText(10, d.height - 25, "3D Viewer - New Panel System", 1.0f, 1.0f, 1.0f);
    if (core && core->modelLoaded) {
        std::string s = "Model: " + core->modelPath.substr(core->modelPath.find_last_of("/\\") + 1);
        renderer.drawText(10, d.height - 40, s, 0.5f, 0.8f, 0.5f);
    } else {
        renderer.drawText(10, d.height - 40, "No model loaded", 1.0f, 0.8f, 0.3f);
    }
    renderer.drawText(d.width - 150, d.height - 25, "ESC to exit", 0.7f, 0.7f, 0.7f);
    renderer.drawText(10, d.height - 55, "Drag title to move | Drag edges to resize", 0.7f, 0.7f, 0.7f);
    
    renderer.restoreMatrices();
    renderer.restoreState(prog, vp, dt);
}

void InterfaceManager::renderDynamic() {
    Dimensions d = getDimensions();
    if (d.width == 0 || d.height == 0) return;
    GLint prog, vp[4];
    GLboolean dt;
    renderer.saveState(prog, vp, dt);
    renderer.setup2D(d.width, d.height);
    renderer.restoreMatrices();
    renderer.restoreState(prog, vp, dt);
}

void InterfaceManager::handleClick(int x, int y) {
    Dimensions d = getDimensions();
    auto p = panelManager->getAt(x, y);
    if (p && p->closeClicked(x, y)) { p->setVisible(false); return; }
    objectUI.handleClick(x, y, d.width, d.height, panels);
}

void InterfaceManager::handleMouseDown(int x, int y) {
    Dimensions d = getDimensions();
    if (d.width == 0 || d.height == 0) return;
    panelManager->onMouseDown(x, y);
    if (panelManager->isDragging() && window) SetCapture(window->getHWND());
}

void InterfaceManager::handleMouseMove(int x, int y) {
    Dimensions d = getDimensions();
    if (d.width == 0 || d.height == 0) return;
    panelManager->onMouseMove(x, y);
    if (panelManager->isDragging()) return;
    
    for (auto p : panelManager->getAll()) {
        if (!p->isVisible()) continue;
        int e = p->getEdge(x, y);
        if (e != -1) {
            SetCursor(LoadCursor(NULL, (e == 0 || e == 1) ? IDC_SIZEWE : IDC_SIZENS));
            return;
        }
    }
    SetCursor(LoadCursor(NULL, IDC_ARROW));
}

void InterfaceManager::handleMouseUp(int x, int y) {
    panelManager->onMouseUp(x, y);
    if (window) ReleaseCapture();
}

void InterfaceManager::SwapFlag(Core &A) { A.isStart = !A.isStart; }
HWND InterfaceManager::getHWND() const { return window ? window->getHWND() : nullptr; }
bool InterfaceManager::CheckerClickToPanel(int x, int y) { return panelManager->getAt(x, y) != nullptr; }
void InterfaceManager::BlockMoveToMainWindow(int x, int y) {}
HCURSOR InterfaceManager::getCursorForEdge(PanelType e) const { return LoadCursor(NULL, IDC_ARROW); }
void InterfaceManager::updatePanelMinSizes() {}
void InterfaceManager::renderMenuBar() {}
void InterfaceManager::handleMenuClick(int x, int y) {}