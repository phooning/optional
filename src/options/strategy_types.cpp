#include "strategy_types.hpp"

namespace options_calculator {

StrategyLeg OptionLeg(PositionSide side, OptionType type, int quantity, double strike, double premium, int daysToExpiry) {
    StrategyLeg leg;
    leg.instrumentType = InstrumentType::Option;
    leg.side = side;
    leg.optionType = type;
    leg.quantity = quantity;
    leg.strike = strike;
    leg.premium = premium;
    leg.currentMarketPremium = premium;
    leg.daysToExpiry = daysToExpiry;
    return leg;
}

StrategyLeg StockLeg(PositionSide side, int shares, double entryPrice) {
    StrategyLeg leg;
    leg.instrumentType = InstrumentType::Stock;
    leg.side = side;
    leg.quantity = shares;
    leg.strike = entryPrice;
    leg.premium = 0.0;
    leg.daysToExpiry = 0;
    return leg;
}

StrategyInstance MakeStrategyInstance(const StrategyTemplate& strategyTemplate) {
    StrategyInstance instance;
    instance.name = strategyTemplate.name;
    instance.category = strategyTemplate.category;
    instance.legs = strategyTemplate.defaultLegs;
    instance.allowStockLegs = strategyTemplate.allowStockLegs;
    instance.allowMultiExpiry = strategyTemplate.allowMultiExpiry;
    return instance;
}

const char* StrategyCategoryDisplayName(StrategyCategory category) {
    switch (category) {
    case StrategyCategory::Basic:
        return "Basic";
    case StrategyCategory::CreditSpreads:
        return "Credit Spreads";
    case StrategyCategory::DebitSpreads:
        return "Debit Spreads";
    case StrategyCategory::Neutral:
        return "Neutral";
    case StrategyCategory::CalendarSpreads:
        return "Calendar Spreads";
    case StrategyCategory::Directional:
        return "Directional";
    case StrategyCategory::Other:
        return "Other";
    case StrategyCategory::Naked:
        return "Naked";
    case StrategyCategory::Ladders:
        return "Ladders";
    case StrategyCategory::RatioSpreads:
        return "Ratio Spreads";
    case StrategyCategory::Synthetic:
        return "Synthetic";
    case StrategyCategory::Arbitrage:
        return "Arbitrage";
    }

    return "Other";
}

} // namespace options_calculator
