#pragma once

#include <optional>
#include <string>
#include <vector>

namespace options_calculator {

enum class InstrumentType {
    Stock,
    Option
};

enum class OptionType {
    Call,
    Put
};

enum class PositionSide {
    Long,
    Short
};

enum class StrategyCategory {
    Basic,
    CreditSpreads,
    DebitSpreads,
    Neutral,
    CalendarSpreads,
    Directional,
    Other,
    Naked,
    Ladders,
    RatioSpreads,
    Synthetic,
    Arbitrage
};

struct MarketInputs {
    double underlyingPrice = 100.0;
    double impliedVolatilityPercent = 25.0;
    double riskFreeRatePercent = 4.5;
    double dividendYieldPercent = 0.0;
    int sharesPerContract = 100;
};

struct StrategyLeg {
    InstrumentType instrumentType = InstrumentType::Option;
    PositionSide side = PositionSide::Long;
    OptionType optionType = OptionType::Call;
    int quantity = 1;
    double strike = 100.0;
    double premium = 0.0;
    int daysToExpiry = 30;
    bool enabled = true;
    bool useCurrentMarketPremium = false;
    double currentMarketPremium = 0.0;
};

struct StrategyTemplate {
    const char* name = "";
    StrategyCategory category = StrategyCategory::Other;
    std::vector<StrategyLeg> defaultLegs;
    bool allowStockLegs = false;
    bool allowMultiExpiry = false;
};

struct StrategyInstance {
    std::string name;
    StrategyCategory category = StrategyCategory::Other;
    std::vector<StrategyLeg> legs;
    bool allowStockLegs = false;
    bool allowMultiExpiry = false;
};

struct PayoffCurve {
    std::string label;
    std::vector<double> spots;
    std::vector<double> values;
};

struct StrategyResult {
    double netEntryCost = 0.0;
    double currentTheoreticalValue = 0.0;
    double currentPL = 0.0;
    double intrinsicValue = 0.0;
    double expirationPLAtCurrentSpot = 0.0;
    std::vector<double> breakevens;
    std::optional<double> maxProfit;
    std::optional<double> maxLoss;
    PayoffCurve expirationCurve;
    PayoffCurve theoreticalCurveToday;
    std::vector<PayoffCurve> futureCurves;
    std::optional<double> currentMarketValue;
    std::optional<double> markToMarketPL;
    std::optional<double> modelEdge;
};

StrategyLeg OptionLeg(PositionSide side, OptionType type, int quantity, double strike, double premium, int daysToExpiry);
StrategyLeg StockLeg(PositionSide side, int shares, double entryPrice);
StrategyInstance MakeStrategyInstance(const StrategyTemplate& strategyTemplate);

const char* StrategyCategoryDisplayName(StrategyCategory category);

} // namespace options_calculator
