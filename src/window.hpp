#pragma once

#include <imgui.h>

struct GLFWwindow;

class TradingWindow {
public:
    TradingWindow();
    ~TradingWindow();

    TradingWindow(const TradingWindow&) = delete;
    TradingWindow& operator=(const TradingWindow&) = delete;
    
    void run();

private:
    GLFWwindow* m_window = nullptr;
    float m_nativeScale = 1.0f;
    ImGuiID m_rootDockspaceId = 0;
    ImGuiStyle m_baseStyle;

    static void glfwErrorCallback(int error, const char* description);
    static void glfwWindowContentScaleCallback(GLFWwindow* window, float xscale, float yscale);
    static void glfwWindowSizeCallback(GLFWwindow* window, int width, int height);
    static void glfwWindowPosCallback(GLFWwindow* window, int x, int y);

    void initGLFW();
    void initImGui();
    void setupCallbacks();
    void applyStyle(float scale);
    void rebuildFonts(bool recreateTexture);

    void frameBegin();
    void frame();
    void frameEnd();
    void recoverImGuiFrame();

    void drawMainMenu();
    void drawRootDockspace();
    void drawPanels();
};