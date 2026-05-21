#include "strategy_registry.hpp"

#include <cstring>

namespace options_calculator {
namespace {

const std::vector<StrategyTemplate>& Registry() {
    static const std::vector<StrategyTemplate> strategies = {
        {
            "Long Call",
            StrategyCategory::Basic,
            {OptionLeg(PositionSide::Long, OptionType::Call, 1, 105.0, 3.50, 30)}
        },
        {
            "Long Put",
            StrategyCategory::Basic,
            {OptionLeg(PositionSide::Long, OptionType::Put, 1, 95.0, 3.25, 30)}
        },
        {
            "Covered Call",
            StrategyCategory::Basic,
            {
                StockLeg(PositionSide::Long, 100, 100.0),
                OptionLeg(PositionSide::Short, OptionType::Call, 1, 105.0, 2.50, 30)
            },
            true
        },
        {
            "Cash-Secured Put",
            StrategyCategory::Basic,
            {OptionLeg(PositionSide::Short, OptionType::Put, 1, 95.0, 2.75, 30)}
        },
        {
            "Bull Call Spread",
            StrategyCategory::DebitSpreads,
            {
                OptionLeg(PositionSide::Long, OptionType::Call, 1, 100.0, 6.00, 30),
                OptionLeg(PositionSide::Short, OptionType::Call, 1, 110.0, 2.50, 30)
            }
        },
        {
            "Bear Put Spread",
            StrategyCategory::DebitSpreads,
            {
                OptionLeg(PositionSide::Long, OptionType::Put, 1, 105.0, 6.00, 30),
                OptionLeg(PositionSide::Short, OptionType::Put, 1, 95.0, 2.50, 30)
            }
        },
        {
            "Bull Put Spread",
            StrategyCategory::CreditSpreads,
            {
                OptionLeg(PositionSide::Short, OptionType::Put, 1, 100.0, 4.25, 30),
                OptionLeg(PositionSide::Long, OptionType::Put, 1, 90.0, 1.50, 30)
            }
        },
        {
            "Iron Condor",
            StrategyCategory::Neutral,
            {
                OptionLeg(PositionSide::Long, OptionType::Put, 1, 85.0, 0.90, 30),
                OptionLeg(PositionSide::Short, OptionType::Put, 1, 90.0, 1.80, 30),
                OptionLeg(PositionSide::Short, OptionType::Call, 1, 110.0, 1.80, 30),
                OptionLeg(PositionSide::Long, OptionType::Call, 1, 115.0, 0.90, 30)
            }
        },
        {
            "Iron Butterfly",
            StrategyCategory::Neutral,
            {
                OptionLeg(PositionSide::Long, OptionType::Put, 1, 90.0, 1.50, 30),
                OptionLeg(PositionSide::Short, OptionType::Put, 1, 100.0, 4.25, 30),
                OptionLeg(PositionSide::Short, OptionType::Call, 1, 100.0, 4.25, 30),
                OptionLeg(PositionSide::Long, OptionType::Call, 1, 110.0, 1.50, 30)
            }
        },
        {
            "Straddle",
            StrategyCategory::Directional,
            {
                OptionLeg(PositionSide::Long, OptionType::Call, 1, 100.0, 4.00, 30),
                OptionLeg(PositionSide::Long, OptionType::Put, 1, 100.0, 4.00, 30)
            }
        },
        {
            "Strangle",
            StrategyCategory::Directional,
            {
                OptionLeg(PositionSide::Long, OptionType::Put, 1, 95.0, 2.50, 30),
                OptionLeg(PositionSide::Long, OptionType::Call, 1, 105.0, 2.50, 30)
            }
        },
        {
            "Collar",
            StrategyCategory::Other,
            {
                StockLeg(PositionSide::Long, 100, 100.0),
                OptionLeg(PositionSide::Long, OptionType::Put, 1, 95.0, 2.75, 30),
                OptionLeg(PositionSide::Short, OptionType::Call, 1, 110.0, 2.25, 30)
            },
            true
        },
        {
            "Protective Put",
            StrategyCategory::CreditSpreads,
            {
                StockLeg(PositionSide::Long, 100, 100.0),
                OptionLeg(PositionSide::Long, OptionType::Put, 1, 95.0, 2.75, 30)
            },
            true
        },
        {
            "Calendar Call Spread",
            StrategyCategory::CalendarSpreads,
            {
                OptionLeg(PositionSide::Short, OptionType::Call, 1, 100.0, 3.00, 30),
                OptionLeg(PositionSide::Long, OptionType::Call, 1, 100.0, 5.50, 60)
            },
            false,
            true
        },
        {
            "Diagonal Call Spread",
            StrategyCategory::CalendarSpreads,
            {
                OptionLeg(PositionSide::Short, OptionType::Call, 1, 105.0, 2.50, 30),
                OptionLeg(PositionSide::Long, OptionType::Call, 1, 100.0, 6.00, 60)
            },
            false,
            true
        },
        {
            "Bull Call Ladder",
            StrategyCategory::Ladders,
            {
                OptionLeg(PositionSide::Long, OptionType::Call, 1, 100.0, 6.00, 30),
                OptionLeg(PositionSide::Short, OptionType::Call, 1, 110.0, 2.50, 30),
                OptionLeg(PositionSide::Short, OptionType::Call, 1, 115.0, 1.35, 30)
            }
        },
        {
            "Bear Put Ladder",
            StrategyCategory::Ladders,
            {
                OptionLeg(PositionSide::Long, OptionType::Put, 1, 100.0, 5.50, 30),
                OptionLeg(PositionSide::Short, OptionType::Put, 1, 90.0, 2.10, 30),
                OptionLeg(PositionSide::Short, OptionType::Put, 1, 85.0, 1.20, 30)
            }
        }
    };

    return strategies;
}

} // namespace

const std::vector<StrategyTemplate>& GetStrategyTemplates() {
    return Registry();
}

std::vector<const StrategyTemplate*> GetStrategiesByCategory(StrategyCategory category) {
    std::vector<const StrategyTemplate*> matches;
    for (const StrategyTemplate& strategyTemplate : Registry()) {
        if (strategyTemplate.category == category) {
            matches.push_back(&strategyTemplate);
        }
    }

    return matches;
}

const StrategyTemplate* FindStrategyTemplate(const char* name) {
    if (name == nullptr) {
        return nullptr;
    }

    for (const StrategyTemplate& strategyTemplate : Registry()) {
        if (std::strcmp(strategyTemplate.name, name) == 0) {
            return &strategyTemplate;
        }
    }

    return nullptr;
}

} // namespace options_calculator
