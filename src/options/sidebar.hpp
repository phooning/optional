#pragma once

namespace options_calculator {

struct StrategyCategory {
    const char* name;
    const char* const* entries;
    int entryCount;
};

const char* DefaultStrategyName();
const StrategyCategory* StrategyCategories(int* categoryCount);
void DrawStrategySidebar(const char** selectedStrategy);

}
