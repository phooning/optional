#include "strategy_ui.hpp"

#include "payoff_plot.hpp"
#include "strategy_eval.hpp"

#include <algorithm>
#include <cstdio>
#include <imgui.h>
#include <string>
#include <unordered_map>

namespace options_calculator {
namespace {

struct StrategySession {
    StrategyInstance strategy;
    MarketInputs market;
};

double ClampDouble(double value, double minValue, double maxValue) {
    return std::min(std::max(value, minValue), maxValue);
}

std::string FormatCurrency(double value) {
    char amount[64];
    std::snprintf(amount, sizeof(amount), "%.2f", std::abs(value));

    std::string formatted(amount);
    const std::size_t decimalIndex = formatted.find('.');
    const std::size_t integerEnd = decimalIndex == std::string::npos ? formatted.size() : decimalIndex;
    for (std::size_t commaIndex = integerEnd; commaIndex > 3; commaIndex -= 3) {
        formatted.insert(commaIndex - 3, ",");
    }

    return value < -0.005 ? "-$" + formatted : "$" + formatted;
}

std::string FormatOptionalCurrency(const std::optional<double>& value, const char* emptyText) {
    if (!value.has_value()) {
        return emptyText;
    }

    return FormatCurrency(*value);
}

std::string FormatBreakevens(const std::vector<double>& breakevens) {
    if (breakevens.empty()) {
        return "N/A";
    }

    std::string text;
    for (std::size_t index = 0; index < breakevens.size(); ++index) {
        if (index != 0) {
            text += ", ";
        }
        text += FormatCurrency(breakevens[index]);
    }

    return text;
}

void DrawResultRow(const char* label, const std::string& value) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(label);
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(value.c_str());
}

void DrawResultRow(const char* label, double value) {
    DrawResultRow(label, FormatCurrency(value));
}

bool HasMultipleExpirations(const StrategyInstance& strategy) {
    int firstExpiry = -1;
    for (const StrategyLeg& leg : strategy.legs) {
        if (!leg.enabled || leg.instrumentType != InstrumentType::Option) {
            continue;
        }

        if (firstExpiry < 0) {
            firstExpiry = leg.daysToExpiry;
        } else if (firstExpiry != leg.daysToExpiry) {
            return true;
        }
    }

    return false;
}

StrategySession& SessionForTemplate(const StrategyTemplate& strategyTemplate) {
    static std::unordered_map<std::string, StrategySession> sessions;
    StrategySession& session = sessions[strategyTemplate.name];
    if (session.strategy.name.empty()) {
        session.strategy = MakeStrategyInstance(strategyTemplate);
    }

    return session;
}

} // namespace

void DrawMarketInputs(MarketInputs& market) {
    ImGui::TextUnformatted("Market");
    ImGui::Separator();

    ImGui::SetNextItemWidth(160.0f);
    ImGui::InputDouble("Underlying price", &market.underlyingPrice, 0.50, 5.0, "%.2f");
    ImGui::SetNextItemWidth(160.0f);
    ImGui::InputDouble("Implied volatility %", &market.impliedVolatilityPercent, 0.25, 2.50, "%.2f");
    ImGui::SetNextItemWidth(160.0f);
    ImGui::InputDouble("Risk-free rate %", &market.riskFreeRatePercent, 0.10, 0.50, "%.2f");
    ImGui::SetNextItemWidth(160.0f);
    ImGui::InputDouble("Dividend yield %", &market.dividendYieldPercent, 0.10, 0.50, "%.2f");
    ImGui::SetNextItemWidth(160.0f);
    ImGui::InputInt("Shares per contract", &market.sharesPerContract);

    market.underlyingPrice = std::max(0.01, market.underlyingPrice);
    market.impliedVolatilityPercent = std::max(0.01, market.impliedVolatilityPercent);
    market.dividendYieldPercent = std::max(0.0, market.dividendYieldPercent);
    market.sharesPerContract = std::max(1, market.sharesPerContract);
}

void DrawLegEditor(StrategyLeg& leg, int legIndex) {
    ImGui::PushID(legIndex);

    const char* sideItems[] = {"Long", "Short"};
    int selectedSide = leg.side == PositionSide::Long ? 0 : 1;
    const char* instrumentItems[] = {"Stock", "Option"};
    int selectedInstrument = leg.instrumentType == InstrumentType::Stock ? 0 : 1;
    const char* optionItems[] = {"Call", "Put"};
    int selectedOptionType = leg.optionType == OptionType::Call ? 0 : 1;

    char title[64];
    std::snprintf(title, sizeof(title), "Leg %d", legIndex + 1);
    if (ImGui::TreeNodeEx(title, ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Enabled", &leg.enabled);

        ImGui::SetNextItemWidth(130.0f);
        if (ImGui::Combo("Side", &selectedSide, sideItems, IM_ARRAYSIZE(sideItems))) {
            leg.side = selectedSide == 0 ? PositionSide::Long : PositionSide::Short;
        }

        ImGui::SetNextItemWidth(130.0f);
        if (ImGui::Combo("Instrument", &selectedInstrument, instrumentItems, IM_ARRAYSIZE(instrumentItems))) {
            leg.instrumentType = selectedInstrument == 0 ? InstrumentType::Stock : InstrumentType::Option;
        }

        if (leg.instrumentType == InstrumentType::Option) {
            ImGui::SetNextItemWidth(130.0f);
            if (ImGui::Combo("Type", &selectedOptionType, optionItems, IM_ARRAYSIZE(optionItems))) {
                leg.optionType = selectedOptionType == 0 ? OptionType::Call : OptionType::Put;
            }
        }

        ImGui::SetNextItemWidth(160.0f);
        ImGui::InputDouble(leg.instrumentType == InstrumentType::Stock ? "Entry price" : "Strike", &leg.strike, 0.50, 5.0, "%.2f");

        if (leg.instrumentType == InstrumentType::Option) {
            ImGui::SetNextItemWidth(160.0f);
            ImGui::InputDouble("Entry premium", &leg.premium, 0.05, 0.50, "%.2f");
            ImGui::Checkbox("Use current market premium", &leg.useCurrentMarketPremium);
            if (leg.useCurrentMarketPremium) {
                ImGui::SetNextItemWidth(160.0f);
                ImGui::InputDouble("Current market premium", &leg.currentMarketPremium, 0.05, 0.50, "%.2f");
            }

            ImGui::SetNextItemWidth(160.0f);
            ImGui::InputInt("Days to expiration", &leg.daysToExpiry);
        }

        ImGui::SetNextItemWidth(160.0f);
        ImGui::InputInt(leg.instrumentType == InstrumentType::Stock ? "Shares" : "Contracts", &leg.quantity);

        leg.quantity = std::max(0, leg.quantity);
        leg.strike = std::max(0.01, leg.strike);
        leg.premium = std::max(0.0, leg.premium);
        leg.currentMarketPremium = std::max(0.0, leg.currentMarketPremium);
        leg.daysToExpiry = std::max(0, leg.daysToExpiry);

        ImGui::TreePop();
    }

    ImGui::PopID();
}

void DrawStrategyEditor(StrategyInstance& strategy) {
    ImGui::TextUnformatted("Legs");
    ImGui::Separator();

    for (int legIndex = 0; legIndex < static_cast<int>(strategy.legs.size()); ++legIndex) {
        DrawLegEditor(strategy.legs[legIndex], legIndex);
    }

    if (ImGui::Button("Add option leg")) {
        strategy.legs.push_back(OptionLeg(PositionSide::Long, OptionType::Call, 1, 100.0, 1.00, 30));
    }

    if (strategy.allowStockLegs) {
        ImGui::SameLine();
        if (ImGui::Button("Add stock leg")) {
            strategy.legs.push_back(StockLeg(PositionSide::Long, 100, 100.0));
        }
    }
}

void DrawStrategyResults(const StrategyResult& result) {
    if (ImGui::BeginTable("StrategyResults", 2, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Metric", ImGuiTableColumnFlags_WidthFixed, 270.0f);
        ImGui::TableSetupColumn("Value");

        DrawResultRow(result.netEntryCost >= 0.0 ? "Net debit" : "Net credit", std::abs(result.netEntryCost));
        DrawResultRow("Current theoretical value", result.currentTheoreticalValue);
        DrawResultRow("Current P/L", result.currentPL);
        DrawResultRow("Intrinsic value", result.intrinsicValue);
        DrawResultRow("Expiration P/L at current spot", result.expirationPLAtCurrentSpot);
        DrawResultRow("Breakevens", FormatBreakevens(result.breakevens));
        DrawResultRow("Max profit", FormatOptionalCurrency(result.maxProfit, "Unlimited"));
        DrawResultRow("Max loss", FormatOptionalCurrency(result.maxLoss, "Unlimited"));

        if (result.currentMarketValue.has_value()) {
            DrawResultRow("Current mark", *result.currentMarketValue);
            DrawResultRow("Mark-to-market P/L", *result.markToMarketPL);
            DrawResultRow("Model edge", *result.modelEdge);
        } else {
            DrawResultRow("Current mark", "Not provided");
        }

        std::string returnOnRisk = "N/A";
        if (result.maxLoss.has_value() && *result.maxLoss > 0.0) {
            const double risk = *result.maxLoss;
            char buffer[64];
            std::snprintf(buffer, sizeof(buffer), "%.2f%%", (result.currentPL / risk) * 100.0);
            returnOnRisk = buffer;
        }
        DrawResultRow("Return on risk", returnOnRisk);

        ImGui::EndTable();
    }
}

void DrawStrategyPage(const StrategyTemplate& strategyTemplate) {
    StrategySession& session = SessionForTemplate(strategyTemplate);

    ImGui::TextUnformatted(session.strategy.name.c_str());
    ImGui::Separator();
    ImGui::Spacing();

    DrawMarketInputs(session.market);
    ImGui::Spacing();
    DrawStrategyEditor(session.strategy);

    if (session.strategy.allowMultiExpiry && HasMultipleExpirations(session.strategy)) {
        ImGui::Spacing();
        ImGui::TextWrapped(
            "This strategy has multiple expirations. Expiration payoff is shown using each leg's configured expiry "
            "model; real-world P/L depends on term structure, assignment risk, and volatility changes."
        );
    }

    StrategyResult result = EvaluateStrategy(session.strategy, session.market);

    ImGui::Spacing();
    DrawStrategyResults(result);
    ImGui::Spacing();
    DrawPayoffPlot(session.strategy, session.market, result);
}

} // namespace options_calculator
