#include "hello_imgui/hello_imgui_include_opengl.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <algorithm>
#include <cstdio>

struct AppWindowState {
    int windowX = 0;
    int windowY = 0;
    int windowWidth = 1600;
    int windowHeight = 1000;
    int frameBufferWidth = 1600;
    int frameBufferHeight = 1000;
    float windowScale = 1.0f;
    bool focused = false;
    bool minimized = false;
    double lastFrameTime = 0.0;

    void Update(GLFWwindow* window) {
        glfwGetWindowPos(window, &windowX, &windowY);
        glfwGetWindowSize(window, &windowWidth, &windowHeight);
        glfwGetFramebufferSize(window, &frameBufferWidth, &frameBufferHeight);

        float scaleX = 1.0f;
        float scaleY = 1.0f;
        glfwGetWindowContentScale(window, &scaleX, &scaleY);
        windowScale = std::max(scaleX, scaleY);

        focused = glfwGetWindowAttrib(window, GLFW_FOCUSED) == GLFW_TRUE;
        minimized = glfwGetWindowAttrib(window, GLFW_ICONIFIED) == GLFW_TRUE;
        lastFrameTime = glfwGetTime();
    }
};

static AppWindowState g_AppWindowState;
static ImGuiID g_RootDockspaceId = 0;

static void GlfwErrorCallback(int error, const char* description) {
    std::fprintf(stderr, "GLFW error %d: %s\n", error, description);
}

static void SetupDeskStyle() {
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding = 4.0f;
    style.WindowPadding = ImVec2(10, 10);
    style.FramePadding = ImVec2(8, 5);
    style.ItemSpacing = ImVec2(8, 6);
    style.ScrollbarSize = 16.0f;
}

static bool PrepareWindowFrame(GLFWwindow* window) {
    g_AppWindowState.Update(window);

    if (g_AppWindowState.minimized) {
        glfwWaitEvents();
        return false;
    }

    if (g_AppWindowState.frameBufferWidth <= 0 || g_AppWindowState.frameBufferHeight <= 0) {
        glfwWaitEvents();
        return false;
    }

    return true;
}

static void BeginFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

static void DrawMainMenu() {
    if (!ImGui::BeginMenuBar()) {
        return;
    }

    if (ImGui::BeginMenu("File")) {
        ImGui::MenuItem("Open");
        if (ImGui::MenuItem("Exit")) {
            GLFWwindow* window = glfwGetCurrentContext();
            if (window != nullptr) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
        }
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

static void DrawRootDockspace() {
    g_RootDockspaceId = ImGui::GetID("RootDockspace");
    ImGui::DockSpace(g_RootDockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
}

static void DrawRootWindow() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_MenuBar;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("RootWindow", nullptr, flags);
    DrawMainMenu();
    DrawRootDockspace();
    ImGui::End();

    ImGui::PopStyleVar(3);
}

static void DrawPanels() {
    if (g_RootDockspaceId != 0) {
        ImGui::SetNextWindowDockID(g_RootDockspaceId, ImGuiCond_FirstUseEver);
    }
    ImGui::Begin("Watchlist");
    ImGui::TextUnformatted("Symbol");
    ImGui::Separator();
    ImGui::TextUnformatted("SPY");
    ImGui::TextUnformatted("QQQ");
    ImGui::TextUnformatted("IWM");
    ImGui::End();

    if (g_RootDockspaceId != 0) {
        ImGui::SetNextWindowDockID(g_RootDockspaceId, ImGuiCond_FirstUseEver);
    }
    ImGui::Begin("Options Chain");
    ImGui::TextUnformatted("Main trading desk area here.");
    ImGui::Separator();
    ImGui::Text("Window: %d,%d  %dx%d  scale %.2f",
        g_AppWindowState.windowX,
        g_AppWindowState.windowY,
        g_AppWindowState.windowWidth,
        g_AppWindowState.windowHeight,
        g_AppWindowState.windowScale);
    ImGui::Text("Focus: %s", g_AppWindowState.focused ? "yes" : "no");
    ImGui::Text("Last frame time: %.3f", g_AppWindowState.lastFrameTime);
    ImGui::End();
}

static void EndFrame(GLFWwindow* window) {
    ImGui::Render();

    glViewport(0, 0, g_AppWindowState.frameBufferWidth, g_AppWindowState.frameBufferHeight);
    glClearColor(0.055f, 0.06f, 0.07f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
}

int main(int, char *[]) {
    glfwSetErrorCallback(GlfwErrorCallback);
    if (!glfwInit()) {
        return 1;
    }

    const char* glslVersion = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow* window = glfwCreateWindow(1600, 1000, "optional", nullptr, nullptr);
    if (window == nullptr) {
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetWindowSizeLimits(window, 960, 640, GLFW_DONT_CARE, GLFW_DONT_CARE);

#if defined(HELLOIMGUI_USE_GLAD)
    if (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)) == 0) {
        std::fprintf(stderr, "Failed to initialize OpenGL loader\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
#endif

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImFontConfig fontConfig;
    fontConfig.SizePixels = 20.0f;
    io.Fonts->Clear();
    io.Fonts->AddFontDefault(&fontConfig);
    io.FontGlobalScale = 1.0f;

    SetupDeskStyle();
    ImGui::GetStyle().ScaleAllSizes(1.25f);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glslVersion);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        if (!PrepareWindowFrame(window)) {
            continue;
        }

        BeginFrame();
        DrawRootWindow();
        DrawPanels();
        EndFrame(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
