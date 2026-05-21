#pragma once

namespace options_calculator {

struct SidebarStrategyCategory {
    const char* name;
    const char* const* entries;
    int entryCount;
};

const char* DefaultStrategyName();
const SidebarStrategyCategory* StrategyCategories(int* categoryCount);
void DrawStrategySidebar(const char** selectedStrategy);

}
