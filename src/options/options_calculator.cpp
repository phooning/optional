#include "options_calculator.hpp"

#include "sidebar.hpp"
#include "strategy_registry.hpp"
#include "strategy_ui.hpp"

#include <imgui.h>

namespace options_calculator {

void Draw(ImGuiID dockspaceId) {
    static const char* selectedStrategy = DefaultStrategyName();

    if (dockspaceId != 0) ImGui::SetNextWindowDockID(dockspaceId, ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Options Calculator")) {
        const float sidebarWidth = 245.0f;
        ImGui::BeginChild("StrategySidebar", ImVec2(sidebarWidth, 0.0f), true);
        DrawStrategySidebar(&selectedStrategy);
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("StrategyPage", ImVec2(0.0f, 0.0f), false);
        const StrategyTemplate* selectedTemplate = FindStrategyTemplate(selectedStrategy);
        if (selectedTemplate != nullptr) {
            DrawStrategyPage(*selectedTemplate);
        }
        ImGui::EndChild();
    }

    ImGui::End();
}

} // namespace options_calculator
