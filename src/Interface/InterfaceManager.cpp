// InterfaceManager.cpp - FULL
#include "InterfaceManager.h"
#include "../Core/core.h"
#include "../Core/Vulkan.h"
#include "../Core/SecondComplexity/Asset/AssetManager.h"
#include "../Core/SecondComplexity/Scene/SceneManager.h"
#include "BufferLayer.h"
#include "../Core/SecondComplexity/Icon/IconManager.h"
#include <iostream>

InterfaceManager::InterfaceManager(Core* corePtr, RenderAPI api) 
    : window(nullptr), core(corePtr), currentAPI(api), startupActive(false),
      renderer(api == RenderAPI::OPENGL ? RenderAPIType::OPENGL : RenderAPIType::VULKAN)
{
    panels = new PanelManager();
    
    int sw = 1280, sh = 720;
    
    panels->add("TopBar", 0, 0, sw, 40);
    panels->add("Hierarchy", 0, 40, 220, sh - 40);
    panels->add("Inspector", sw - 260, 40, 260, sh - 40);
    panels->add("3D Viewport", 220, 40, sw - 480, sh - 40, true);
    panels->add("Asset Browser", 220, sh - 200, sw - 480, 200);
    
    panels->registerCallback("Load Model", [this]() { 
        if (core) core->openFileDialogAndLoadModel(getHWND()); 
    });
    panels->registerCallback("Start", [this]() { 
        if (core) SwapFlag(*core); 
    });
    panels->registerCallback("Stop", [this]() { 
        if (core) SwapFlag(*core); 
    });
    panels->registerCallback("Apply", []() { 
        std::cout << "Apply" << std::endl; 
    });
    panels->registerCallback("Reset", []() { 
        std::cout << "Reset" << std::endl; 
    });
    panels->registerCallback("Wireframe", []() { 
        std::cout << "Wireframe" << std::endl; 
    });
    panels->registerCallback("Solid", []() { 
        std::cout << "Solid" << std::endl; 
    });
    panels->registerCallback("Clear", []() { 
        std::cout << "Clear" << std::endl; 
    });
    
    auto top = panels->getPanel("TopBar");
    if (top) {
        top->addButton("Load Model", 230, 8, 100, 24, 0.4f, 0.5f, 0.4f, [this]() { 
            if (core) core->openFileDialogAndLoadModel(getHWND()); 
        });
        top->addButton("Start", 340, 8, 60, 24, 0.4f, 0.5f, 0.4f, [this]() { 
            if (core) SwapFlag(*core); 
        });
        top->addButton("Stop", 410, 8, 60, 24, 0.5f, 0.4f, 0.4f, [this]() { 
            if (core) SwapFlag(*core); 
        });
    }
    
    auto hierarchy = panels->getPanel("Hierarchy");
    if (hierarchy) {
        hierarchy->addLabel("Scene Objects:", 10, 130, 12, true, 0.8f, 0.8f, 0.9f);
        hierarchy->addLabel("Click object to select", 10, 150, 10, false, 0.5f, 0.5f, 0.5f);
        hierarchy->addLabel("Selected: None", 10, 170, 11, false, 0.7f, 0.5f, 0.3f);
    }
    
    auto inspector = panels->getPanel("Inspector");
    if (inspector) {
        inspector->addButton("Apply", 10, 35, 80, 24, 0.4f, 0.5f, 0.4f, []() { 
            std::cout << "Apply" << std::endl; 
        });
        inspector->addButton("Reset", 100, 35, 80, 24, 0.5f, 0.4f, 0.4f, []() { 
            std::cout << "Reset" << std::endl; 
        });
    }
    
    auto view3D = panels->getPanel("3D Viewport");
    if (view3D) {
        view3D->addButton("Wireframe", 10, 35, 80, 24, 0.4f, 0.4f, 0.5f, []() { 
            std::cout << "Wireframe" << std::endl; 
        });
        view3D->addButton("Solid", 100, 35, 80, 24, 0.4f, 0.4f, 0.5f, []() { 
            std::cout << "Solid" << std::endl; 
        });
    }
    
    auto assetBrowser = panels->getPanel("Asset Browser");
    if (assetBrowser) {
        assetBrowser->addLabel("Path: /", 10, 35, 12, false, 0.7f, 0.8f, 0.7f);
        assetBrowser->pathLabelIndex = (int)assetBrowser->labels.size() - 1;
        
        assetBrowser->addButton("<", 10, 55, 30, 20, 0.4f, 0.4f, 0.5f, [this]() {
            BufferLayer::Instance().NavigateBack();
        });
        assetBrowser->addButton(">", 45, 55, 30, 20, 0.4f, 0.4f, 0.5f, []() {});
        assetBrowser->addButton("Up", 80, 55, 30, 20, 0.4f, 0.4f, 0.5f, []() {
            BufferLayer::Instance().NavigateUp();
        });
        assetBrowser->addButton("Refresh", 120, 55, 60, 20, 0.4f, 0.5f, 0.4f, []() {
            Asset* root = AssetManager::Instance().GetRootAsset();
            if (root) {
                std::string rootPath = AssetManager::Instance().GetProjectRoot();
                if (!rootPath.empty()) {
                    AssetManager::Instance().LoadProject(rootPath);
                    BufferLayer::Instance().ResetNavigation();
                }
            }
        });
        assetBrowser->addButton("Clear", 190, 55, 50, 20, 0.5f, 0.4f, 0.4f, []() { 
            std::cout << "Clear" << std::endl; 
        });
        assetBrowser->addLabel("> Ready", 10, 85, 12, false, 0.7f, 0.8f, 0.7f);
    }
    
    panels->loadLayout("Config/WindowSettings.json");
}

InterfaceManager::~InterfaceManager() { 
    panels->saveLayout("Config/WindowSettings.json");
    delete panels; 
}

void InterfaceManager::setVulkan(Vulkan* vk) {
    renderer.setVulkan(vk);
    objectUI.setVulkan(vk);  // ДОБАВЬ ЭТУ СТРОКУ
    IconManager::Instance().SetRenderer(&renderer);
}

InterfaceManager::Dimensions InterfaceManager::getDimensions() const {
    Dimensions d = {0, 0};
    if (!window) return d;
    HWND hwnd = window->getHWND();
    if (!hwnd) return d;
    RECT rect;
    GetClientRect(hwnd, &rect);
    d.width = rect.right - rect.left;
    d.height = rect.bottom - rect.top;
    return d;
}

void InterfaceManager::clearScreen(int width, int height) {
    if (currentAPI == RenderAPI::OPENGL) {
        glViewport(0, 0, width, height);
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
}

void InterfaceManager::setup3DViewport(const Dimensions& dims) {
    Panel* view3D = panels ? panels->get3D() : nullptr;
    
    if (currentAPI == RenderAPI::OPENGL) {
        if (view3D && view3D->visible) {
            glViewport(view3D->getX(), dims.height - (view3D->getY() + view3D->getH()), 
                       view3D->getW(), view3D->getH());
        } else {
            glViewport(0, 0, dims.width, dims.height);
        }
        glEnable(GL_DEPTH_TEST);
    }
}

void InterfaceManager::Get3DViewportRect(int& x, int& y, int& w, int& h) const {
    Panel* view3D = panels ? panels->get3D() : nullptr;
    if (view3D && view3D->visible && !view3D->collapsed) {
        x = view3D->getX();
        y = view3D->getY();
        w = view3D->getW();
        h = view3D->getH();
    } else {
        x = 0;
        y = 0;
        Dimensions dims = getDimensions();
        w = dims.width;
        h = dims.height;
    }
}

void InterfaceManager::printIcon(int x, int y, int w, int h, const std::string& iconType, int size) {
    if (currentAPI != RenderAPI::VULKAN) return;
    
    Vulkan* vk = core->getVulkan();
    if (!vk) return;
    
    IconUV uv = IconManager::Instance().GetIconUV(iconType, size);
    if (!uv.textureId) {
        vk->drawQuad(x, y, x + w, y + h, 1.0f, 0.0f, 0.0f);
        return;
    }
    
    vk->drawImageUV(x, y, x + w, y + h, (VulkanTexture*)uv.textureId, uv.u1, uv.v1, uv.u2, uv.v2);
}

void InterfaceManager::renderStatic() {
    Dimensions d = getDimensions();
    if (d.width == 0 || d.height == 0) return;
    
    if (currentAPI == RenderAPI::OPENGL) {
        GLint prog, vp[4];
        GLboolean dt;
        renderer.saveState(prog, vp, dt);
        renderer.setup2D(d.width, d.height);
        
        panels->update(d.width, d.height);
        panels->render(renderer);
        objectUI.render(renderer, d.width, d.height, *panels);
        
        Panel* assetPanel = panels->getPanel("Asset Browser");
        if (assetPanel && assetPanel->visible && !assetPanel->collapsed) {
            Asset* root = AssetManager::Instance().GetRootAsset();
            if (root) {
                Asset* currentDir = BufferLayer::Instance().GetCurrentDirectory();
                if (!currentDir) currentDir = root;
                
                if (assetPanel->pathLabelIndex >= 0 && assetPanel->pathLabelIndex < (int)assetPanel->labels.size()) {
                    std::string displayPath = currentDir->path;
                    std::string rootPath = AssetManager::Instance().GetProjectRoot();
                    if (!rootPath.empty() && displayPath.find(rootPath) == 0) {
                        displayPath = "/" + displayPath.substr(rootPath.length());
                    }
                    if (displayPath.length() > 60) {
                        displayPath = "..." + displayPath.substr(displayPath.length() - 57);
                    }
                    assetPanel->labels[assetPanel->pathLabelIndex].text = "Path: " + displayPath;
                }
                
                int contentX = assetPanel->getX() + 10;
                int contentY = assetPanel->getY() + 105;
                int contentW = assetPanel->getW() - 20;
                int contentH = assetPanel->getH() - 115;
                
                if (contentW > 0 && contentH > 0) {
                    BufferLayer::Instance().VivodAsset(renderer, contentX, contentY, contentW, contentH, this);
                }
            }
        }
        
        // Hierarchy panel
        Panel* hierarchy = panels->getPanel("Hierarchy");
        if (hierarchy && hierarchy->visible && !hierarchy->collapsed) {
            auto& sm = SceneManager::Instance();
            auto objects = sm.GetAllObjects();
            
            int startY = hierarchy->getY() + 195;
            int xOffset = hierarchy->getX() + 10;
            int lineHeight = 18;
            
            SceneObject* selected = sm.GetSelectedObject();
            if (selected) {
                for (auto& lbl : hierarchy->labels) {
                    if (lbl.text.find("Selected:") == 0) {
                        lbl.text = "Selected: " + selected->name;
                        break;
                    }
                }
            } else {
                for (auto& lbl : hierarchy->labels) {
                    if (lbl.text.find("Selected:") == 0) {
                        lbl.text = "Selected: None";
                        break;
                    }
                }
            }
            
            int maxItems = (hierarchy->getH() - (startY - hierarchy->getY()) - 20) / lineHeight;
            if (maxItems < 1) maxItems = 1;
            
            for (size_t i = 0; i < objects.size() && i < (size_t)maxItems; i++) {
                auto obj = objects[i];
                if (!obj) continue;
                
                int yPos = startY + i * lineHeight;
                
                std::string displayName = obj->name;
                if (obj->type == ObjectType::CAMERA) displayName += " [CAM]";
                if (obj->type == ObjectType::MODEL) displayName += " [M]";
                
                if (displayName.length() > 25) {
                    displayName = displayName.substr(0, 22) + "...";
                }
                
                float colorR, colorG, colorB;
                if (obj->selected) {
                    colorR = 1.0f; colorG = 0.5f; colorB = 0.0f;
                    renderer.drawQuad(xOffset, yPos - 2, 
                                     hierarchy->getX() + hierarchy->getW() - 10, 
                                     yPos + 14, 
                                     0.3f, 0.2f, 0.1f);
                } else if (obj->visible) {
                    colorR = 0.7f; colorG = 0.8f; colorB = 0.7f;
                } else {
                    colorR = 0.4f; colorG = 0.4f; colorB = 0.4f;
                }
                
                renderer.drawText(xOffset + 10, yPos, displayName, colorR, colorG, colorB);
            }
            
            char objCount[64];
            sprintf_s(objCount, "Total: %zu objects", objects.size());
            renderer.drawText(xOffset, startY + maxItems * lineHeight + 5, objCount, 0.5f, 0.5f, 0.5f);
        }
        
        renderer.drawText(10, d.height - 25, "3D Viewer", 1.0f, 1.0f, 1.0f);
        if (core && core->modelLoaded) {
            std::string s = "Model: " + core->modelPath.substr(core->modelPath.find_last_of("/\\") + 1);
            renderer.drawText(10, d.height - 40, s, 0.5f, 0.8f, 0.5f);
        } else {
            renderer.drawText(10, d.height - 40, "No model loaded", 1.0f, 0.8f, 0.3f);
        }
        
        renderer.restoreMatrices();
        renderer.restoreState(prog, vp, dt);
    } else {
        renderer.setup2D(d.width, d.height);
        
        panels->update(d.width, d.height);
        panels->render(renderer);
        objectUI.render(renderer, d.width, d.height, *panels);
        
        Panel* assetPanel = panels->getPanel("Asset Browser");
        if (assetPanel && assetPanel->visible && !assetPanel->collapsed) {
            Asset* root = AssetManager::Instance().GetRootAsset();
            if (root) {
                Asset* currentDir = BufferLayer::Instance().GetCurrentDirectory();
                if (!currentDir) currentDir = root;
                
                if (assetPanel->pathLabelIndex >= 0 && assetPanel->pathLabelIndex < (int)assetPanel->labels.size()) {
                    std::string displayPath = currentDir->path;
                    std::string rootPath = AssetManager::Instance().GetProjectRoot();
                    if (!rootPath.empty() && displayPath.find(rootPath) == 0) {
                        displayPath = "/" + displayPath.substr(rootPath.length());
                    }
                    if (displayPath.length() > 60) {
                        displayPath = "..." + displayPath.substr(displayPath.length() - 57);
                    }
                    assetPanel->labels[assetPanel->pathLabelIndex].text = "Path: " + displayPath;
                }
                
                int contentX = assetPanel->getX() + 10;
                int contentY = assetPanel->getY() + 105;
                int contentW = assetPanel->getW() - 20;
                int contentH = assetPanel->getH() - 115;
                
                if (contentW > 0 && contentH > 0) {
                    BufferLayer::Instance().VivodAsset(renderer, contentX, contentY, contentW, contentH, this);
                }
            }
        }
        
        Panel* hierarchy = panels->getPanel("Hierarchy");
        if (hierarchy && hierarchy->visible && !hierarchy->collapsed) {
            auto& sm = SceneManager::Instance();
            auto objects = sm.GetAllObjects();
            
            int startY = hierarchy->getY() + 195;
            int xOffset = hierarchy->getX() + 10;
            int lineHeight = 18;
            
            SceneObject* selected = sm.GetSelectedObject();
            if (selected) {
                for (auto& lbl : hierarchy->labels) {
                    if (lbl.text.find("Selected:") == 0) {
                        lbl.text = "Selected: " + selected->name;
                        break;
                    }
                }
            } else {
                for (auto& lbl : hierarchy->labels) {
                    if (lbl.text.find("Selected:") == 0) {
                        lbl.text = "Selected: None";
                        break;
                    }
                }
            }
            
            int maxItems = (hierarchy->getH() - (startY - hierarchy->getY()) - 20) / lineHeight;
            if (maxItems < 1) maxItems = 1;
            
            for (size_t i = 0; i < objects.size() && i < (size_t)maxItems; i++) {
                auto obj = objects[i];
                if (!obj) continue;
                
                int yPos = startY + i * lineHeight;
                
                std::string displayName = obj->name;
                if (obj->type == ObjectType::CAMERA) displayName += " [CAM]";
                if (obj->type == ObjectType::MODEL) displayName += " [M]";
                
                if (displayName.length() > 25) {
                    displayName = displayName.substr(0, 22) + "...";
                }
                
                float colorR, colorG, colorB;
                if (obj->selected) {
                    colorR = 1.0f; colorG = 0.5f; colorB = 0.0f;
                    renderer.drawQuad(xOffset, yPos - 2, 
                                     hierarchy->getX() + hierarchy->getW() - 10, 
                                     yPos + 14, 
                                     0.3f, 0.2f, 0.1f);
                } else if (obj->visible) {
                    colorR = 0.7f; colorG = 0.8f; colorB = 0.7f;
                } else {
                    colorR = 0.4f; colorG = 0.4f; colorB = 0.4f;
                }
                
                renderer.drawText(xOffset + 10, yPos, displayName, colorR, colorG, colorB);
            }
            
            char objCount[64];
            sprintf_s(objCount, "Total: %zu objects", objects.size());
            renderer.drawText(xOffset, startY + maxItems * lineHeight + 5, objCount, 0.5f, 0.5f, 0.5f);
        }
        
        if (testTexture && testTexture->valid) {
            Vulkan* vk = core->getVulkan();
            if (vk) {
                int imgW = 400, imgH = 400;
                int imgX = (d.width - imgW) / 2;
                int imgY = (d.height - imgH) / 2;
                vk->drawImage(imgX, imgY, imgX + imgW, imgY + imgH, testTexture);
            }
        }
        
        renderer.drawText(10, d.height - 25, "3D Viewer (Vulkan)", 1.0f, 1.0f, 1.0f);
        if (core && core->modelLoaded) {
            std::string s = "Model: " + core->modelPath.substr(core->modelPath.find_last_of("/\\") + 1);
            renderer.drawText(10, d.height - 40, s, 0.5f, 0.8f, 0.5f);
        } else {
            renderer.drawText(10, d.height - 40, "No model loaded", 1.0f, 0.8f, 0.3f);
        }
    }
}

void InterfaceManager::renderDynamic() {
    Dimensions d = getDimensions();
    if (d.width == 0 || d.height == 0) return;
    
    if (currentAPI == RenderAPI::OPENGL) {
        GLint prog, vp[4];
        GLboolean dt;
        renderer.saveState(prog, vp, dt);
        renderer.setup2D(d.width, d.height);
        renderer.restoreMatrices();
        renderer.restoreState(prog, vp, dt);
    }
}

void InterfaceManager::handleRightClick(int x, int y) {
    Panel* assetPanel = panels->getPanel("Asset Browser");
    if (assetPanel && assetPanel->visible && !assetPanel->collapsed) {
        int contentX = assetPanel->getX() + 10;
        int contentY = assetPanel->getY() + 105;
        int contentW = assetPanel->getW() - 20;
        int contentH = assetPanel->getH() - 115;
        
        if (x >= contentX && x <= contentX + contentW && y >= contentY && y <= contentY + contentH) {
            Asset* clicked = BufferLayer::Instance().GetAssetAtPosition(x, y);
            
            BufferLayer::Instance().showContextMenu = true;
            BufferLayer::Instance().menuX = x;
            BufferLayer::Instance().menuY = y;
            BufferLayer::Instance().selectedAsset = clicked;
            
            if (clicked) {
                std::cout << "[UI] Right-click on: " << clicked->name << std::endl;
            } else {
                std::cout << "[UI] Right-click on empty space" << std::endl;
            }
        }
    }
}

void InterfaceManager::handleClick(int x, int y) {
    isClick = true;
    
    // Сначала передаём клик в ObjectUI для полей ввода
    objectUI.handleClick(x, y, 0, 0, *panels);
    
    // Если поле ввода активно - не обрабатываем другие клики
    if (objectUI.isAnyInputActive()) return;
    
    // Проверяем клик в панели Hierarchy для выбора объекта
    Panel* hierarchy = panels->getPanel("Hierarchy");
    if (hierarchy && hierarchy->visible && !hierarchy->collapsed) {
        int startY = hierarchy->getY() + 195;
        int xOffset = hierarchy->getX() + 10;
        int lineHeight = 18;
        
        auto& sm = SceneManager::Instance();
        auto objects = sm.GetAllObjects();
        
        for (size_t i = 0; i < objects.size(); i++) {
            int yPos = startY + i * lineHeight;
            
            if (x >= xOffset && x <= hierarchy->getX() + hierarchy->getW() - 10 &&
                y >= yPos - 2 && y <= yPos + 14) {
                
                SceneObject* clicked = objects[i];
                if (clicked) {
                    if (sm.GetSelectedObject()) {
                        sm.GetSelectedObject()->selected = false;
                    }
                    sm.SelectObject(clicked->id);
                    clicked->selected = true;
                    std::cout << "[UI] Selected object: " << clicked->name << " (ID: " << clicked->id << ")" << std::endl;
                } else {
                    sm.DeselectObject();
                    std::cout << "[UI] Deselected object" << std::endl;
                }
                return;
            }
        }
    }
    
    Panel* assetPanel = panels->getPanel("Asset Browser");
    if (assetPanel && assetPanel->visible && !assetPanel->collapsed) {
        if (assetPanel->onClickButton(x, y)) {
            return;
        }
        
        int contentX = assetPanel->getX() + 10;
        int contentY = assetPanel->getY() + 105;
        int contentW = assetPanel->getW() - 20;
        int contentH = assetPanel->getH() - 115;
        
        if (x >= contentX && x <= contentX + contentW && y >= contentY && y <= contentY + contentH) {
            Asset* clicked = BufferLayer::Instance().GetAssetAtPosition(x, y);
            
            static auto lastClick = std::chrono::steady_clock::now();
            auto now = std::chrono::steady_clock::now();
            bool doubleClick = (now - lastClick) < std::chrono::milliseconds(300);
            lastClick = now;
            
            if (clicked) {
                if (doubleClick) {
                    if (clicked->isFolder) {
                        BufferLayer::Instance().NavigateTo(clicked);
                    } else {
                        BufferLayer::Instance().OpenAssetExternal(clicked);
                    }
                }
            }
        }
    }
}

void InterfaceManager::handleMouseDown(int x, int y) { 
    panels->onMouseDown(x, y); 
}

void InterfaceManager::handleMouseMove(int x, int y) { 
    panels->onMouseMove(x, y);
    isClick = false;
    
    for (auto p : panels->getAll()) {
        if (!p->visible) continue;
        int edge = p->getEdge(x, y, 10);
        if (edge == 0 || edge == 1) {
            SetCursor(LoadCursor(NULL, IDC_SIZEWE));
            return;
        } else if (edge == 2 || edge == 3) {
            SetCursor(LoadCursor(NULL, IDC_SIZENS));
            return;
        }
    }
    SetCursor(LoadCursor(NULL, IDC_ARROW));
}

void InterfaceManager::handleMouseUp(int x, int y) { 
    panels->onMouseUp(x, y); 
}

void InterfaceManager::SwapFlag(Core &A) { 
    A.isStart = !A.isStart; 
}

HWND InterfaceManager::getHWND() const { 
    return window ? window->getHWND() : nullptr; 
}

void InterfaceManager::BlockMoveToMainWindow(int x, int y) {}

bool InterfaceManager::CheckerClickToPanel(int x, int y) { 
    return panels->at(x, y) != nullptr; 
}

bool InterfaceManager::isBlockingRender() {
    return false;
}

void InterfaceManager::setWindow(InitialWin32* win) {
    window = win;
}

void InterfaceManager::initializeRender(HWND hwnd, int width, int height) {
    renderer.initialize(hwnd, width, height);
}

void InterfaceManager::updateWindowSize(int width, int height) {
    renderer.setup2D(width, height);
}
