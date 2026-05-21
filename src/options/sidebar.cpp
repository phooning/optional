#include "sidebar.hpp"

#include "strategy_registry.hpp"

#include <array>
#include <imgui.h>
#include <vector>

namespace options_calculator {
namespace {

constexpr std::array<StrategyCategory, 12> kCategoryOrder = {
    StrategyCategory::Basic,
    StrategyCategory::CreditSpreads,
    StrategyCategory::DebitSpreads,
    StrategyCategory::Neutral,
    StrategyCategory::CalendarSpreads,
    StrategyCategory::Directional,
    StrategyCategory::Other,
    StrategyCategory::Naked,
    StrategyCategory::Ladders,
    StrategyCategory::RatioSpreads,
    StrategyCategory::Synthetic,
    StrategyCategory::Arbitrage,
};

std::vector<const char*> StrategyNamesForCategory(StrategyCategory category) {
    std::vector<const char*> names;
    for (const StrategyTemplate* strategyTemplate : GetStrategiesByCategory(category)) {
        names.push_back(strategyTemplate->name);
    }

    return names;
}

} // namespace

const char* DefaultStrategyName() {
    const std::vector<StrategyTemplate>& strategies = GetStrategyTemplates();
    return strategies.empty() ? "" : strategies.front().name;
}

const SidebarStrategyCategory* StrategyCategories(int* categoryCount) {
    static std::array<std::vector<const char*>, kCategoryOrder.size()> entriesByCategory;
    static std::array<SidebarStrategyCategory, kCategoryOrder.size()> sidebarCategories;
    static bool initialized = false;

    if (!initialized) {
        for (std::size_t categoryIndex = 0; categoryIndex < kCategoryOrder.size(); ++categoryIndex) {
            entriesByCategory[categoryIndex] = StrategyNamesForCategory(kCategoryOrder[categoryIndex]);
            sidebarCategories[categoryIndex] = {
                StrategyCategoryDisplayName(kCategoryOrder[categoryIndex]),
                entriesByCategory[categoryIndex].data(),
                static_cast<int>(entriesByCategory[categoryIndex].size())
            };
        }
        initialized = true;
    }

    if (categoryCount != nullptr) {
        *categoryCount = static_cast<int>(sidebarCategories.size());
    }
    return sidebarCategories.data();
}

void DrawStrategySidebar(const char** selectedStrategy) {
    ImGui::TextUnformatted("Strategies");
    ImGui::Separator();

    int categoryCount = 0;
    const SidebarStrategyCategory* categories = StrategyCategories(&categoryCount);
    for (int categoryIndex = 0; categoryIndex < categoryCount; ++categoryIndex) {
        const SidebarStrategyCategory& category = categories[categoryIndex];
        if (category.entryCount == 0) {
            continue;
        }

        if (!ImGui::CollapsingHeader(category.name, ImGuiTreeNodeFlags_DefaultOpen)) {
            continue;
        }

        ImGui::PushID(category.name);
        for (int entryIndex = 0; entryIndex < category.entryCount; ++entryIndex) {
            const char* entry = category.entries[entryIndex];
            const bool selected = selectedStrategy != nullptr && *selectedStrategy == entry;
            if (ImGui::Selectable(entry, selected) && selectedStrategy != nullptr) {
                *selectedStrategy = entry;
            }
        }
        ImGui::PopID();
    }
}

} // namespace options_calculator
