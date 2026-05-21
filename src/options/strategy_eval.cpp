#include "strategy_eval.hpp"

#include "option_math.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace options_calculator {
namespace {

constexpr int kCurvePointCount = 240;
constexpr double kSlopeEpsilon = 0.000001;

double SideMultiplier(PositionSide side) {
    return side == PositionSide::Long ? 1.0 : -1.0;
}

double LegContractMultiplier(const StrategyLeg& leg, int sharesPerContract) {
    if (leg.instrumentType == InstrumentType::Stock) {
        return static_cast<double>(std::max(0, leg.quantity));
    }

    return static_cast<double>(std::max(0, leg.quantity)) * static_cast<double>(std::max(1, sharesPerContract));
}

double EvaluateLegEntryCost(const StrategyLeg& leg, int sharesPerContract) {
    if (!leg.enabled) {
        return 0.0;
    }

    const double multiplier = LegContractMultiplier(leg, sharesPerContract);
    const double entryPrice = leg.instrumentType == InstrumentType::Stock ? leg.strike : leg.premium;
    return SideMultiplier(leg.side) * entryPrice * multiplier;
}

double EvaluateLegIntrinsicPositionValue(const StrategyLeg& leg, double spot, int sharesPerContract) {
    if (!leg.enabled) {
        return 0.0;
    }

    const double multiplier = LegContractMultiplier(leg, sharesPerContract);
    if (leg.instrumentType == InstrumentType::Stock) {
        return SideMultiplier(leg.side) * spot * multiplier;
    }

    return SideMultiplier(leg.side) * OptionIntrinsicValue(leg.optionType, spot, leg.strike) * multiplier;
}

double EvaluateLegMarkedValue(const StrategyLeg& leg, double spot, const MarketInputs& market) {
    if (!leg.enabled) {
        return 0.0;
    }

    if (leg.instrumentType == InstrumentType::Stock) {
        return EvaluateLegTheoreticalValue(leg, spot, market);
    }

    const double optionValue = leg.useCurrentMarketPremium ?
        std::max(0.0, leg.currentMarketPremium) :
        OptionTheoreticalValue(
            leg.optionType,
            spot,
            leg.strike,
            leg.daysToExpiry,
            market.impliedVolatilityPercent,
            market.riskFreeRatePercent,
            market.dividendYieldPercent
        );

    return SideMultiplier(leg.side) * optionValue * LegContractMultiplier(leg, market.sharesPerContract);
}

bool HasAnyMarketMarks(const StrategyInstance& strategy) {
    for (const StrategyLeg& leg : strategy.legs) {
        if (leg.enabled && leg.instrumentType == InstrumentType::Option && leg.useCurrentMarketPremium) {
            return true;
        }
    }

    return false;
}

double MaximumStrikeOrEntry(const StrategyInstance& strategy) {
    double maximum = 0.0;
    for (const StrategyLeg& leg : strategy.legs) {
        if (leg.enabled) {
            maximum = std::max(maximum, leg.strike);
        }
    }

    return maximum;
}

double PlotUpperBound(const StrategyInstance& strategy, const MarketInputs& market) {
    return std::max({1.0, market.underlyingPrice * 2.0, MaximumStrikeOrEntry(strategy) * 1.75});
}

double EvaluateStrategyExpirationPLAtSpot(const StrategyInstance& strategy, double spot, int sharesPerContract) {
    double value = 0.0;
    for (const StrategyLeg& leg : strategy.legs) {
        value += EvaluateLegExpirationPL(leg, spot, sharesPerContract);
    }

    return value;
}

double EvaluateStrategyTheoreticalValueAtSpot(const StrategyInstance& strategy, double spot, const MarketInputs& market) {
    double value = 0.0;
    for (const StrategyLeg& leg : strategy.legs) {
        value += EvaluateLegTheoreticalValue(leg, spot, market);
    }

    return value;
}

double EvaluateStrategyEntryCost(const StrategyInstance& strategy, int sharesPerContract) {
    double value = 0.0;
    for (const StrategyLeg& leg : strategy.legs) {
        value += EvaluateLegEntryCost(leg, sharesPerContract);
    }

    return value;
}

double EvaluateStrategyIntrinsicValueAtSpot(const StrategyInstance& strategy, double spot, int sharesPerContract) {
    double value = 0.0;
    for (const StrategyLeg& leg : strategy.legs) {
        value += EvaluateLegIntrinsicPositionValue(leg, spot, sharesPerContract);
    }

    return value;
}

double EvaluateUpsideSlope(const StrategyInstance& strategy, int sharesPerContract) {
    double slope = 0.0;
    for (const StrategyLeg& leg : strategy.legs) {
        if (!leg.enabled) {
            continue;
        }

        const double side = SideMultiplier(leg.side);
        if (leg.instrumentType == InstrumentType::Stock) {
            slope += side * static_cast<double>(std::max(0, leg.quantity));
        } else if (leg.optionType == OptionType::Call) {
            slope += side * static_cast<double>(std::max(0, leg.quantity)) * static_cast<double>(std::max(1, sharesPerContract));
        }
    }

    return slope;
}

PayoffCurve BuildExpirationCurve(const StrategyInstance& strategy, const MarketInputs& market, double xMax) {
    PayoffCurve curve;
    curve.label = "Payoff at expiration";
    curve.spots.resize(kCurvePointCount);
    curve.values.resize(kCurvePointCount);

    for (int pointIndex = 0; pointIndex < kCurvePointCount; ++pointIndex) {
        const double t = static_cast<double>(pointIndex) / static_cast<double>(kCurvePointCount - 1);
        const double spot = xMax * t;
        curve.spots[pointIndex] = spot;
        curve.values[pointIndex] = EvaluateStrategyExpirationPLAtSpot(strategy, spot, market.sharesPerContract);
    }

    return curve;
}

PayoffCurve BuildTheoreticalCurve(const StrategyInstance& strategy, const MarketInputs& market, double xMax, const char* label) {
    PayoffCurve curve;
    curve.label = label;
    curve.spots.resize(kCurvePointCount);
    curve.values.resize(kCurvePointCount);

    const double netEntryCost = EvaluateStrategyEntryCost(strategy, market.sharesPerContract);
    for (int pointIndex = 0; pointIndex < kCurvePointCount; ++pointIndex) {
        const double t = static_cast<double>(pointIndex) / static_cast<double>(kCurvePointCount - 1);
        const double spot = xMax * t;
        curve.spots[pointIndex] = spot;
        curve.values[pointIndex] = EvaluateStrategyTheoreticalValueAtSpot(strategy, spot, market) - netEntryCost;
    }

    return curve;
}

StrategyInstance StrategyAfterDays(const StrategyInstance& strategy, int elapsedDays) {
    StrategyInstance adjusted = strategy;
    for (StrategyLeg& leg : adjusted.legs) {
        if (leg.instrumentType == InstrumentType::Option) {
            leg.daysToExpiry = std::max(0, leg.daysToExpiry - elapsedDays);
        }
    }

    return adjusted;
}

std::vector<double> FindBreakevens(const PayoffCurve& curve) {
    std::vector<double> breakevens;
    if (curve.spots.size() != curve.values.size() || curve.spots.size() < 2) {
        return breakevens;
    }

    for (std::size_t index = 1; index < curve.spots.size(); ++index) {
        const double previousValue = curve.values[index - 1];
        const double currentValue = curve.values[index];
        if (std::abs(previousValue) < 0.0001) {
            breakevens.push_back(curve.spots[index - 1]);
            continue;
        }

        if ((previousValue < 0.0 && currentValue > 0.0) || (previousValue > 0.0 && currentValue < 0.0)) {
            const double denominator = previousValue - currentValue;
            if (std::abs(denominator) > 0.000001) {
                const double t = previousValue / denominator;
                breakevens.push_back(curve.spots[index - 1] + (curve.spots[index] - curve.spots[index - 1]) * t);
            }
        }
    }

    if (!curve.values.empty() && std::abs(curve.values.back()) < 0.0001) {
        breakevens.push_back(curve.spots.back());
    }

    return breakevens;
}

} // namespace

double EvaluateLegExpirationPL(const StrategyLeg& leg, double spot, int sharesPerContract) {
    if (!leg.enabled) {
        return 0.0;
    }

    const double multiplier = LegContractMultiplier(leg, sharesPerContract);
    if (leg.instrumentType == InstrumentType::Stock) {
        return SideMultiplier(leg.side) * (spot - leg.strike) * multiplier;
    }

    const double intrinsicValue = OptionExpirationPayoff(leg.optionType, spot, leg.strike);
    const double perSharePL = leg.side == PositionSide::Long ?
        intrinsicValue - leg.premium :
        leg.premium - intrinsicValue;
    return perSharePL * multiplier;
}

double EvaluateLegTheoreticalValue(const StrategyLeg& leg, double spot, const MarketInputs& market) {
    if (!leg.enabled) {
        return 0.0;
    }

    const double multiplier = LegContractMultiplier(leg, market.sharesPerContract);
    if (leg.instrumentType == InstrumentType::Stock) {
        return SideMultiplier(leg.side) * spot * multiplier;
    }

    const double optionValue = OptionTheoreticalValue(
        leg.optionType,
        spot,
        leg.strike,
        leg.daysToExpiry,
        market.impliedVolatilityPercent,
        market.riskFreeRatePercent,
        market.dividendYieldPercent
    );

    return SideMultiplier(leg.side) * optionValue * multiplier;
}

StrategyResult EvaluateStrategy(const StrategyInstance& strategy, const MarketInputs& market) {
    StrategyResult result;
    const double xMax = PlotUpperBound(strategy, market);

    result.netEntryCost = EvaluateStrategyEntryCost(strategy, market.sharesPerContract);
    result.currentTheoreticalValue = EvaluateStrategyTheoreticalValueAtSpot(strategy, market.underlyingPrice, market);
    result.currentPL = result.currentTheoreticalValue - result.netEntryCost;
    result.intrinsicValue = EvaluateStrategyIntrinsicValueAtSpot(strategy, market.underlyingPrice, market.sharesPerContract);
    result.expirationPLAtCurrentSpot =
        EvaluateStrategyExpirationPLAtSpot(strategy, market.underlyingPrice, market.sharesPerContract);

    result.expirationCurve = BuildExpirationCurve(strategy, market, xMax);
    result.theoreticalCurveToday = BuildTheoreticalCurve(strategy, market, xMax, "Theoretical P/L today");

    constexpr std::array<int, 4> kForwardDays = {1, 7, 14, 30};
    for (int forwardDays : kForwardDays) {
        bool hasValidOption = false;
        for (const StrategyLeg& leg : strategy.legs) {
            if (leg.enabled && leg.instrumentType == InstrumentType::Option && leg.daysToExpiry > forwardDays) {
                hasValidOption = true;
                break;
            }
        }

        if (!hasValidOption) {
            continue;
        }

        StrategyInstance adjusted = StrategyAfterDays(strategy, forwardDays);
        PayoffCurve curve = BuildTheoreticalCurve(adjusted, market, xMax, "");
        curve.label = "Theoretical P/L +" + std::to_string(forwardDays) + (forwardDays == 1 ? " day" : " days");
        result.futureCurves.push_back(curve);
    }

    result.breakevens = FindBreakevens(result.expirationCurve);

    auto minmax = std::minmax_element(result.expirationCurve.values.begin(), result.expirationCurve.values.end());
    const double upsideSlope = EvaluateUpsideSlope(strategy, market.sharesPerContract);
    if (upsideSlope > kSlopeEpsilon) {
        result.maxProfit = std::nullopt;
    } else if (minmax.second != result.expirationCurve.values.end()) {
        result.maxProfit = std::max(0.0, *minmax.second);
    }

    if (upsideSlope < -kSlopeEpsilon) {
        result.maxLoss = std::nullopt;
    } else if (minmax.first != result.expirationCurve.values.end()) {
        result.maxLoss = std::max(0.0, -*minmax.first);
    }

    if (HasAnyMarketMarks(strategy)) {
        double markedValue = 0.0;
        for (const StrategyLeg& leg : strategy.legs) {
            markedValue += EvaluateLegMarkedValue(leg, market.underlyingPrice, market);
        }
        result.currentMarketValue = markedValue;
        result.markToMarketPL = markedValue - result.netEntryCost;
        result.modelEdge = result.currentTheoreticalValue - markedValue;
    }

    return result;
}

} // namespace options_calculator
