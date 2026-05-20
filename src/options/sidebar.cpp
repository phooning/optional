#include "sidebar.hpp"

#include <imgui.h>

namespace options_calculator {
namespace {

constexpr const char* kBasic[] = {
    "Long Call",
    "Long Put",
    "Covered Call",
    "Cash-Secured Put",
};

constexpr const char* kCreditSpreads[] = {
    "Protective Put",
    "Bull Put Spread",
};

constexpr const char* kDebitSpreads[] = {
    "Bull Call Spread",
    "Bear Put Spread",
};

constexpr const char* kNeutral[] = {
    "Iron Butterfly",
    "Iron Condor",
    "Long Put Butterfly",
    "Long Call Butterfly",
    "Short Straddle",
    "Short Strangle",
    "Long Call Condor",
    "Long Put Condor",
    "Covered Short Straddle",
    "Covered Short Strangle",
};

constexpr const char* kCalendarSpreads[] = {
    "Calendar Call Spread",
    "Calendar Put Spread",
    "Diagonal Call Spread",
    "Diagonal Put Spread",
};

constexpr const char* kDirectional[] = {
    "Inverse Iron Butterfly",
    "Inverse Iron Condor",
    "Short Put Butterfly",
    "Straddle",
    "Strangle",
    "Short Call Condor",
    "Short Put Condor",
};

constexpr const char* kOther[] = {
    "Collar",
    "Jade Lizard",
    "Reverse Jade Lizard",
    "Strip",
    "Strap",
    "Guts",
    "Short Guts",
    "Double Diagonal",
};

constexpr const char* kNaked[] = {
    "Short Put",
    "Short Call",
};

constexpr const char* kLaddars[] = {
    "Bull Call Laddar",
    "Bear Call Ladder",
    "Bull Put Ladder",
    "Bear Put Laddar",
};

constexpr const char* kRatioSpreads[] = {
    "Call Ratio Backspread",
    "Put Broken Wing",
    "Inverse Call Broken Wing",
    "Put Ratio Backspread",
    "Call Broken Wing",
    "Inverse Put Broken Wing",
    "Call Ratio Spread",
    "Put Ratio Spread",
};

constexpr const char* kSynthetic[] = {
    "Long Synthetic Future",
    "Short Synthetic Future",
    "Synthetic Put",
};

constexpr const char* kArbitrage[] = {
    "Long Combo",
    "Short Combo",
};

constexpr StrategyCategory kCategories[] = {
    {"Basic", kBasic, IM_ARRAYSIZE(kBasic)},
    {"Credit Spreads", kCreditSpreads, IM_ARRAYSIZE(kCreditSpreads)},
    {"Debit Spreads", kDebitSpreads, IM_ARRAYSIZE(kDebitSpreads)},
    {"Neutral", kNeutral, IM_ARRAYSIZE(kNeutral)},
    {"Calendar Spreads", kCalendarSpreads, IM_ARRAYSIZE(kCalendarSpreads)},
    {"Directional", kDirectional, IM_ARRAYSIZE(kDirectional)},
    {"Other", kOther, IM_ARRAYSIZE(kOther)},
    {"Naked", kNaked, IM_ARRAYSIZE(kNaked)},
    {"Laddars", kLaddars, IM_ARRAYSIZE(kLaddars)},
    {"Ratio Spreads", kRatioSpreads, IM_ARRAYSIZE(kRatioSpreads)},
    {"Synthetic", kSynthetic, IM_ARRAYSIZE(kSynthetic)},
    {"Arbitrage", kArbitrage, IM_ARRAYSIZE(kArbitrage)},
};

} // namespace

const char* DefaultStrategyName() {
    return kBasic[0];
}

const StrategyCategory* StrategyCategories(int* categoryCount) {
    if (categoryCount != nullptr) {
        *categoryCount = IM_ARRAYSIZE(kCategories);
    }
    return kCategories;
}

void DrawStrategySidebar(const char** selectedStrategy) {
    ImGui::TextUnformatted("Strategies");
    ImGui::Separator();

    int categoryCount = 0;
    const StrategyCategory* categories = StrategyCategories(&categoryCount);
    for (int categoryIndex = 0; categoryIndex < categoryCount; ++categoryIndex) {
        const StrategyCategory& category = categories[categoryIndex];
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
