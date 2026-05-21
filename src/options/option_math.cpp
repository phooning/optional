#include "option_math.hpp"

#include <algorithm>
#include <cmath>

namespace options_calculator {
namespace {

constexpr double kCalendarDaysPerYear = 365.0;
constexpr double kSqrtTwo = 1.4142135623730951;
constexpr double kInverseSqrtTwoPi = 0.3989422804014327;

double NormalPdf(double value) {
    return kInverseSqrtTwoPi * std::exp(-0.5 * value * value);
}

bool CanUseBlackScholes(double spot, double strike, double yearsToExpiry, double volatility) {
    return yearsToExpiry > 0.0 && volatility > 0.0 && spot > 0.0 && strike > 0.0;
}

double BlackScholesD1(
    double spot,
    double strike,
    double yearsToExpiry,
    double volatility,
    double riskFreeRate,
    double dividendYield
) {
    const double sigmaSqrtT = volatility * std::sqrt(yearsToExpiry);
    return (std::log(spot / strike) + (riskFreeRate - dividendYield + 0.5 * volatility * volatility) * yearsToExpiry) /
        sigmaSqrtT;
}

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
    if (!CanUseBlackScholes(spot, strike, yearsToExpiry, volatility)) {
        return OptionIntrinsicValue(OptionType::Call, spot, strike);
    }

    const double sigmaSqrtT = volatility * std::sqrt(yearsToExpiry);
    const double d1 = BlackScholesD1(spot, strike, yearsToExpiry, volatility, riskFreeRate, dividendYield);
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
    if (!CanUseBlackScholes(spot, strike, yearsToExpiry, volatility)) {
        return OptionIntrinsicValue(OptionType::Put, spot, strike);
    }

    const double sigmaSqrtT = volatility * std::sqrt(yearsToExpiry);
    const double d1 = BlackScholesD1(spot, strike, yearsToExpiry, volatility, riskFreeRate, dividendYield);
    const double d2 = d1 - sigmaSqrtT;

    return strike * std::exp(-riskFreeRate * yearsToExpiry) * NormalCdf(-d2) -
        spot * std::exp(-dividendYield * yearsToExpiry) * NormalCdf(-d1);
}

double BlackScholesCallDelta(
    double spot,
    double strike,
    double yearsToExpiry,
    double volatility,
    double riskFreeRate,
    double dividendYield
) {
    if (!CanUseBlackScholes(spot, strike, yearsToExpiry, volatility)) {
        return spot > strike ? 1.0 : 0.0;
    }

    // Call delta: e^(-qT) * N(d1)
    const double d1 = BlackScholesD1(spot, strike, yearsToExpiry, volatility, riskFreeRate, dividendYield);
    return std::exp(-dividendYield * yearsToExpiry) * NormalCdf(d1);
}

double BlackScholesPutDelta(
    double spot,
    double strike,
    double yearsToExpiry,
    double volatility,
    double riskFreeRate,
    double dividendYield
) {
    if (!CanUseBlackScholes(spot, strike, yearsToExpiry, volatility)) {
        return spot < strike ? -1.0 : 0.0;
    }

    // Put delta: e^(-qT) * (N(d1) - 1)
    const double d1 = BlackScholesD1(spot, strike, yearsToExpiry, volatility, riskFreeRate, dividendYield);
    return std::exp(-dividendYield * yearsToExpiry) * (NormalCdf(d1) - 1.0);
}

double BlackScholesCallTheta(
    double spot,
    double strike,
    double yearsToExpiry,
    double volatility,
    double riskFreeRate,
    double dividendYield
) {
    if (!CanUseBlackScholes(spot, strike, yearsToExpiry, volatility)) {
        return 0.0;
    }

    const double sqrtT = std::sqrt(yearsToExpiry);
    const double d1 = BlackScholesD1(spot, strike, yearsToExpiry, volatility, riskFreeRate, dividendYield);
    const double d2 = d1 - volatility * sqrtT;

    // Call theta: -(S e^(-qT) phi(d1) sigma)/(2 sqrt(T)) - r K e^(-rT) N(d2) + q S e^(-qT) N(d1)
    return -(spot * std::exp(-dividendYield * yearsToExpiry) * NormalPdf(d1) * volatility) / (2.0 * sqrtT) -
        riskFreeRate * strike * std::exp(-riskFreeRate * yearsToExpiry) * NormalCdf(d2) +
        dividendYield * spot * std::exp(-dividendYield * yearsToExpiry) * NormalCdf(d1);
}

double BlackScholesPutTheta(
    double spot,
    double strike,
    double yearsToExpiry,
    double volatility,
    double riskFreeRate,
    double dividendYield
) {
    if (!CanUseBlackScholes(spot, strike, yearsToExpiry, volatility)) {
        return 0.0;
    }

    const double sqrtT = std::sqrt(yearsToExpiry);
    const double d1 = BlackScholesD1(spot, strike, yearsToExpiry, volatility, riskFreeRate, dividendYield);
    const double d2 = d1 - volatility * sqrtT;

    // Put theta: -(S e^(-qT) phi(d1) sigma)/(2 sqrt(T)) + r K e^(-rT) N(-d2) - q S e^(-qT) N(-d1)
    return -(spot * std::exp(-dividendYield * yearsToExpiry) * NormalPdf(d1) * volatility) / (2.0 * sqrtT) +
        riskFreeRate * strike * std::exp(-riskFreeRate * yearsToExpiry) * NormalCdf(-d2) -
        dividendYield * spot * std::exp(-dividendYield * yearsToExpiry) * NormalCdf(-d1);
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
