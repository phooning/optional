#pragma once

#include "strategy_types.hpp"

#include <vector>

namespace options_calculator {

const std::vector<StrategyTemplate>& GetStrategyTemplates();
std::vector<const StrategyTemplate*> GetStrategiesByCategory(StrategyCategory category);
const StrategyTemplate* FindStrategyTemplate(const char* name);

} // namespace options_calculator
