#include "panels.hpp"

#include "options/options_calculator.hpp"

#include <imgui.h>

namespace panels {
namespace {

void DrawWatchlist(ImGuiID dockspaceId) {
    if (dockspaceId != 0) ImGui::SetNextWindowDockID(dockspaceId, ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Watchlist")) {
        ImGui::TextUnformatted("Symbol");
        ImGui::Separator();
        ImGui::TextUnformatted("SPY");
        ImGui::TextUnformatted("QQQ");
        ImGui::TextUnformatted("IWM");
    }
    ImGui::End();
}

} // namespace

void Draw(ImGuiID dockspaceId) {
    DrawWatchlist(dockspaceId);
    options_calculator::Draw(dockspaceId);
}

} // namespace panels
