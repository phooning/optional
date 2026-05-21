#pragma once

#include "strategy_types.hpp"

namespace options_calculator {

double EvaluateLegExpirationPL(const StrategyLeg& leg, double spot, int sharesPerContract);
double EvaluateLegTheoreticalValue(const StrategyLeg& leg, double spot, const MarketInputs& market);
StrategyResult EvaluateStrategy(const StrategyInstance& strategy, const MarketInputs& market);

} // namespace options_calculator
