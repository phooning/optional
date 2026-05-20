#include "hello_imgui/hello_imgui_include_opengl.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include "window.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <exception>

TradingWindow::TradingWindow() {
    initGLFW();
    initImGui();
    setupCallbacks();
}

TradingWindow::~TradingWindow() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

void TradingWindow::glfwErrorCallback(int error, const char* description) {
    std::fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

void TradingWindow::initGLFW() {
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) std::exit(1);

#if defined(__APPLE__)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);

    m_window = glfwCreateWindow(1600, 1000, "Trading Desk", nullptr, nullptr);
    if (!m_window) {
        glfwTerminate();
        std::exit(1);
    }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1); // VSync (set to 0 for unlimited FPS)

#if defined(HELLOIMGUI_USE_GLAD)
    if (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)) == 0) {
        std::fprintf(stderr, "Failed to initialize OpenGL loader\n");
        glfwDestroyWindow(m_window);
        glfwTerminate();
        std::exit(1);
    }
#endif

    float xscale = 1.0f;
    float yscale = 1.0f;
    glfwGetWindowContentScale(m_window, &xscale, &yscale);
    m_nativeScale = std::max(xscale, yscale);
    glfwSetWindowSizeLimits(m_window, 960, 640, GLFW_DONT_CARE, GLFW_DONT_CARE);
}

void TradingWindow::initImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_NavEnableKeyboard;

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 4.0f;
    style.WindowPadding = ImVec2(10, 10);
    style.FramePadding = ImVec2(8, 5);
    style.ItemSpacing = ImVec2(8, 6);
    style.ScrollbarSize = 16.0f;
    m_baseStyle = style;
    applyStyle(1.25f);

    ImGui_ImplGlfw_InitForOpenGL(m_window, true);

#if defined(__APPLE__)
    ImGui_ImplOpenGL3_Init("#version 150");
#else
    ImGui_ImplOpenGL3_Init("#version 130");
#endif

    rebuildFonts(false);
}

void TradingWindow::setupCallbacks() {
    glfwSetWindowUserPointer(m_window, this);
    glfwSetWindowContentScaleCallback(m_window, glfwWindowContentScaleCallback);
    glfwSetWindowSizeCallback(m_window, glfwWindowSizeCallback);
    glfwSetWindowPosCallback(m_window, glfwWindowPosCallback);
}

void TradingWindow::applyStyle(float scale) {
    ImGui::GetStyle() = m_baseStyle;
    ImGui::GetStyle().ScaleAllSizes(scale);
}

void TradingWindow::rebuildFonts(bool recreateTexture) {
    ImGuiIO& io = ImGui::GetIO();

    ImFontConfig fontConfig;
    fontConfig.SizePixels = 16.0f * m_nativeScale;

    io.Fonts->Clear();
    io.Fonts->AddFontDefault(&fontConfig);
    io.FontGlobalScale = 1.0f;

    if (recreateTexture) {
        glfwMakeContextCurrent(m_window);
        ImGui_ImplOpenGL3_DestroyDeviceObjects();
        ImGui_ImplOpenGL3_CreateDeviceObjects();
    }
}

void TradingWindow::glfwWindowContentScaleCallback(GLFWwindow* window, float xscale, float yscale) {
    auto* self = static_cast<TradingWindow*>(glfwGetWindowUserPointer(window));
    if (!self) return;

    float oldScale = self->m_nativeScale;
    self->m_nativeScale = std::max(xscale, yscale);
    if (oldScale <= 0.0f || self->m_nativeScale <= 0.0f) return;

    self->applyStyle(1.25f * self->m_nativeScale);
    self->rebuildFonts(true);
}

void TradingWindow::glfwWindowSizeCallback(GLFWwindow*, int, int) {}
void TradingWindow::glfwWindowPosCallback(GLFWwindow*, int, int) {}

void TradingWindow::frameBegin() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_MenuBar;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    if (ImGui::Begin("RootDockspaceWindow", nullptr, flags)) {
        ImGui::PopStyleVar(3);
        drawMainMenu();
        drawRootDockspace();
    } else {
        ImGui::PopStyleVar(3);
    }
    ImGui::End();
}

void TradingWindow::drawMainMenu() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            ImGui::MenuItem("Open");
            if (ImGui::MenuItem("Exit")) glfwSetWindowShouldClose(m_window, GLFW_TRUE);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Watchlist");
            ImGui::MenuItem("Options Chain");
            ImGui::MenuItem("Vol Surface");
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
}

void TradingWindow::drawRootDockspace() {
    static bool firstFrame = true;

    m_rootDockspaceId = ImGui::GetID("MainDockspace");
    if (firstFrame) {
        ImGui::DockBuilderRemoveNode(m_rootDockspaceId);
        ImGui::DockBuilderAddNode(m_rootDockspaceId, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(m_rootDockspaceId, ImGui::GetMainViewport()->Size);

        ImGuiID dockLeft = 0;
        ImGuiID dockRight = 0;
        ImGui::DockBuilderSplitNode(m_rootDockspaceId, ImGuiDir_Left, 0.25f, &dockLeft, &dockRight);

        ImGui::DockBuilderDockWindow("Watchlist", dockLeft);
        ImGui::DockBuilderDockWindow("Options Chain", dockRight);
        ImGui::DockBuilderFinish(m_rootDockspaceId);
        firstFrame = false;
    }

    ImGui::DockSpace(m_rootDockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
}

void TradingWindow::frame() {
    drawPanels();
}

void TradingWindow::drawPanels() {
    ImGuiID dockspaceId = m_rootDockspaceId;

    // === Panels with automatic docking on first run ===
    if (dockspaceId != 0) ImGui::SetNextWindowDockID(dockspaceId, ImGuiCond_FirstUseEver);
    ImGui::Begin("Watchlist");
    ImGui::TextUnformatted("Symbol");
    ImGui::Separator();
    ImGui::TextUnformatted("SPY");
    ImGui::TextUnformatted("QQQ");
    ImGui::TextUnformatted("IWM");
    ImGui::End();

    if (dockspaceId != 0) ImGui::SetNextWindowDockID(dockspaceId, ImGuiCond_FirstUseEver);
    ImGui::Begin("Options Chain");
    ImGui::TextUnformatted("Main trading desk area here.");
    ImGui::Separator();
    ImGui::TextUnformatted("No more stretching on resize.");
    ImGui::End();
}

void TradingWindow::frameEnd() {
    ImGui::Render();

    int fbWidth, fbHeight;
    glfwGetFramebufferSize(m_window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);
    glClearColor(0.055f, 0.06f, 0.07f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(m_window);
}

void TradingWindow::recoverImGuiFrame() {
    ImGuiContext* context = ImGui::GetCurrentContext();
    if (context != nullptr && context->WithinFrameScope) {
        ImGui::EndFrame();
        ImGui::UpdatePlatformWindows();
    }
}

void TradingWindow::run() {
    int consecutiveFrameExceptions = 0;

    while (!glfwWindowShouldClose(m_window)) {
        try {
            glfwPollEvents();
            frameBegin();
            frame();
            frameEnd();
            consecutiveFrameExceptions = 0;
        } catch (const std::exception& exception) {
            recoverImGuiFrame();
            std::fprintf(stderr, "Frame exception: %s\n", exception.what());
            if (++consecutiveFrameExceptions >= 10) {
                std::fprintf(stderr, "Too many consecutive frame exceptions; closing.\n");
                glfwSetWindowShouldClose(m_window, GLFW_TRUE);
            }
        } catch (...) {
            recoverImGuiFrame();
            std::fprintf(stderr, "Unknown frame exception.\n");
            if (++consecutiveFrameExceptions >= 10) {
                std::fprintf(stderr, "Too many consecutive frame exceptions; closing.\n");
                glfwSetWindowShouldClose(m_window, GLFW_TRUE);
            }
        }
    }
}