#pragma once

#include "strategy_types.hpp"

namespace options_calculator {

void DrawPayoffPlot(const StrategyInstance& strategy, const MarketInputs& market, const StrategyResult& result);

} // namespace options_calculator
