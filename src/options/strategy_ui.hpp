#pragma once

#include "strategy_types.hpp"

namespace options_calculator {

void DrawLegEditor(StrategyLeg& leg, int legIndex);
void DrawMarketInputs(MarketInputs& market);
void DrawStrategyEditor(StrategyInstance& strategy);
void DrawStrategyResults(const StrategyResult& result);
void DrawStrategyPage(const StrategyTemplate& strategyTemplate);

} // namespace options_calculator
