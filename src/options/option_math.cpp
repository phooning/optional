#include "option_math.hpp"

#include <algorithm>
#include <cmath>

namespace options_calculator {
namespace {

constexpr double kCalendarDaysPerYear = 365.0;
constexpr double kSqrtTwo = 1.4142135623730951;

} // namespace

double NormalCdf(double value) {
    return 0.5 * std::erfc(-value / kSqrtTwo);
}

double YearsToExpiryFromCalendarDays(double days) {
    return std::max(0.0, days) / kCalendarDaysPerYear;
}

double BlackScholesCall(
    double spot,
    double strike,
    double yearsToExpiry,
    double volatility,
    double riskFreeRate,
    double dividendYield
) {
    if (yearsToExpiry <= 0.0 || volatility <= 0.0 || spot <= 0.0 || strike <= 0.0) {
        return OptionIntrinsicValue(OptionType::Call, spot, strike);
    }

    const double sigmaSqrtT = volatility * std::sqrt(yearsToExpiry);
    const double d1 =
        (std::log(spot / strike) + (riskFreeRate - dividendYield + 0.5 * volatility * volatility) * yearsToExpiry) /
        sigmaSqrtT;
    const double d2 = d1 - sigmaSqrtT;

    return spot * std::exp(-dividendYield * yearsToExpiry) * NormalCdf(d1) -
        strike * std::exp(-riskFreeRate * yearsToExpiry) * NormalCdf(d2);
}

double BlackScholesPut(
    double spot,
    double strike,
    double yearsToExpiry,
    double volatility,
    double riskFreeRate,
    double dividendYield
) {
    if (yearsToExpiry <= 0.0 || volatility <= 0.0 || spot <= 0.0 || strike <= 0.0) {
        return OptionIntrinsicValue(OptionType::Put, spot, strike);
    }

    const double sigmaSqrtT = volatility * std::sqrt(yearsToExpiry);
    const double d1 =
        (std::log(spot / strike) + (riskFreeRate - dividendYield + 0.5 * volatility * volatility) * yearsToExpiry) /
        sigmaSqrtT;
    const double d2 = d1 - sigmaSqrtT;

    return strike * std::exp(-riskFreeRate * yearsToExpiry) * NormalCdf(-d2) -
        spot * std::exp(-dividendYield * yearsToExpiry) * NormalCdf(-d1);
}

double OptionIntrinsicValue(OptionType type, double spot, double strike) {
    if (type == OptionType::Call) {
        return std::max(0.0, spot - strike);
    }

    return std::max(0.0, strike - spot);
}

double OptionTheoreticalValue(
    OptionType type,
    double spot,
    double strike,
    int daysToExpiry,
    double impliedVolatilityPercent,
    double riskFreeRatePercent,
    double dividendYieldPercent
) {
    const double yearsToExpiry = YearsToExpiryFromCalendarDays(static_cast<double>(daysToExpiry));
    const double volatility = std::max(0.0, impliedVolatilityPercent) / 100.0;
    const double riskFreeRate = riskFreeRatePercent / 100.0;
    const double dividendYield = std::max(0.0, dividendYieldPercent) / 100.0;

    if (type == OptionType::Call) {
        return BlackScholesCall(spot, strike, yearsToExpiry, volatility, riskFreeRate, dividendYield);
    }

    return BlackScholesPut(spot, strike, yearsToExpiry, volatility, riskFreeRate, dividendYield);
}

double OptionExpirationPayoff(OptionType type, double spot, double strike) {
    return OptionIntrinsicValue(type, spot, strike);
}

} // namespace options_calculator
