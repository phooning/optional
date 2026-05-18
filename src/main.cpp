#include "hello_imgui/hello_imgui.h"
#include <hello_imgui/imgui_window_params.h>
#include <hello_imgui/runner_params.h>
#include <imgui.h>

static void SetupDeskStyle() {
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding = 4.0f;
    style.WindowPadding = ImVec2(10, 10);
    style.FramePadding = ImVec2(8, 5);
    style.ItemSpacing = ImVec2(8, 6);
    style.ScrollbarSize = 16.0f;
}

int main(int, char *[]) {
    HelloImGui::RunnerParams params;

    params.appWindowParams.windowTitle = "optional";
    params.appWindowParams.windowGeometry.sizeAuto = false;
    params.appWindowParams.windowGeometry.size = {1600, 1000};

    params.imGuiWindowParams.defaultImGuiWindowType = HelloImGui::DefaultImGuiWindowType::ProvideFullScreenDockSpace;

    params.imGuiWindowParams.showMenuBar = true;
    params.imGuiWindowParams.showStatusBar = true;

    params.callbacks.LoadAdditionalFonts = [] {
        ImGuiIO& io = ImGui::GetIO();
        ImFontConfig font_cfg;
        font_cfg.SizePixels = 20.0f;

        // Reset fonts because it crashes on Linux.
        io.Fonts->Clear();
        io.Fonts->AddFontDefault(&font_cfg);
        ImGui::GetStyle().ScaleAllSizes(1.25f);
        io.FontGlobalScale = 1.0f;
    };

    params.callbacks.SetupImGuiStyle = [] {
        SetupDeskStyle();
    };


    HelloImGui::Run(params);
    return 0;
}
