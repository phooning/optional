#pragma once

#include "strategy_types.hpp"

namespace options_calculator {

double NormalCdf(double value);
double YearsToExpiryFromCalendarDays(double days);

double BlackScholesCall(
    double spot,
    double strike,
    double yearsToExpiry,
    double volatility,
    double riskFreeRate,
    double dividendYield
);

double BlackScholesPut(
    double spot,
    double strike,
    double yearsToExpiry,
    double volatility,
    double riskFreeRate,
    double dividendYield
);

double OptionIntrinsicValue(OptionType type, double spot, double strike);

double OptionTheoreticalValue(
    OptionType type,
    double spot,
    double strike,
    int daysToExpiry,
    double impliedVolatilityPercent,
    double riskFreeRatePercent,
    double dividendYieldPercent
);

double OptionExpirationPayoff(OptionType type, double spot, double strike);

} // namespace options_calculator
