#include "strategy_ui.hpp"

#include "payoff_plot.hpp"
#include "strategy_eval.hpp"
#include "utils.hpp"

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

std::string FormatNumber(double value) {
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.2f", value);
    return buffer;
}

ImVec4 ProfitLossColor(double value) {
    if (value > 0.005) {
        return ImVec4(0.25f, 0.78f, 0.42f, 1.0f);
    }
    if (value < -0.005) {
        return ImVec4(0.95f, 0.28f, 0.28f, 1.0f);
    }
    return ImGui::GetStyleColorVec4(ImGuiCol_Text);
}

void DrawMetricLabel(const char* label, const char* tooltip) {
    ImGui::TextUnformatted(label);
    if (tooltip != nullptr) {
        ImGui::SetItemTooltip("%s", tooltip);
    }
}

void DrawResultRow(const char* label, const std::string& value, const char* tooltip = nullptr) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    DrawMetricLabel(label, tooltip);
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(value.c_str());
}

void DrawCurrencyRow(const char* label, double value, const char* tooltip = nullptr, bool colorize = false) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    DrawMetricLabel(label, tooltip);
    ImGui::TableNextColumn();
    const std::string formatted = FormatCurrency(value);
    if (colorize) {
        ImGui::TextColored(ProfitLossColor(value), "%s", formatted.c_str());
    } else {
        ImGui::TextUnformatted(formatted.c_str());
    }
}

void DrawNumberRow(const char* label, double value, const char* tooltip = nullptr) {
    DrawResultRow(label, FormatNumber(value), tooltip);
}

void DrawPercentRow(const char* label, double value, const char* tooltip = nullptr) {
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.2f%%", value);
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    DrawMetricLabel(label, tooltip);
    ImGui::TableNextColumn();
    ImGui::TextColored(ProfitLossColor(value), "%s", buffer);
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

void ResetLegForInstrument(StrategyLeg& leg, InstrumentType instrumentType) {
    leg.instrumentType = instrumentType;
    leg.delta = 0.0;
    leg.theta = 0.0;

    if (instrumentType == InstrumentType::Stock) {
        leg.optionType = OptionType::Call;
        leg.premium = 0.0;
        leg.currentMarketPremium = 0.0;
        leg.useCurrentMarketPremium = false;
        leg.daysToExpiry = 0;
        return;
    }

    leg.optionType = OptionType::Call;
    leg.premium = 0.0;
    leg.currentMarketPremium = 0.0;
    leg.useCurrentMarketPremium = false;
    leg.daysToExpiry = 30;
}

const char* InstrumentDisplayName(const StrategyLeg& leg) {
    if (leg.instrumentType == InstrumentType::Stock) {
        return "Stock";
    }

    return leg.optionType == OptionType::Call ? "Call" : "Put";
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
            const InstrumentType newInstrument = selectedInstrument == 0 ? InstrumentType::Stock : InstrumentType::Option;
            if (newInstrument != leg.instrumentType) {
                ResetLegForInstrument(leg, newInstrument);
            }
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
        bool reorderedLeg = false;
        ImGui::PushID(legIndex);
        if (ImGui::Button("^ Up") && legIndex > 0) {
            std::swap(strategy.legs[legIndex], strategy.legs[legIndex - 1]);
            reorderedLeg = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("v Down") && legIndex + 1 < static_cast<int>(strategy.legs.size())) {
            std::swap(strategy.legs[legIndex], strategy.legs[legIndex + 1]);
            reorderedLeg = true;
        }
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.72f, 0.16f, 0.16f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.88f, 0.22f, 0.22f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.56f, 0.10f, 0.10f, 1.0f));
        const bool removeLeg = ImGui::Button("x Remove");
        ImGui::PopStyleColor(3);
        ImGui::PopID();

        if (removeLeg) {
            strategy.legs.erase(strategy.legs.begin() + legIndex);
            --legIndex;
            continue;
        }

        if (reorderedLeg) {
            continue;
        }

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

        DrawCurrencyRow(result.netEntryCost >= 0.0 ? "Net debit" : "Net credit", std::abs(result.netEntryCost));
        DrawCurrencyRow("Current theoretical value", result.currentTheoreticalValue);
        DrawCurrencyRow("Current P/L", result.currentPL, "Theoretical value today minus the original net entry cost.", true);
        DrawCurrencyRow("Intrinsic value", result.intrinsicValue);
        DrawCurrencyRow(
            "Expiration P/L at current spot",
            result.expirationPLAtCurrentSpot,
            "Projected expiration profit or loss if the underlying stayed at the current price.",
            true
        );
        DrawResultRow("Breakevens", FormatBreakevens(result.breakevens));
        DrawResultRow("Max profit", FormatOptionalCurrency(result.maxProfit, "Unlimited"));
        DrawResultRow("Max loss", FormatOptionalCurrency(result.maxLoss, "Unlimited"));

        if (result.currentMarketValue.has_value()) {
            DrawCurrencyRow("Current mark", *result.currentMarketValue);
            DrawCurrencyRow(
                "Mark-to-market P/L",
                *result.markToMarketPL,
                "Current marked value minus the original net entry cost.",
                true
            );
            DrawCurrencyRow(
                "Model edge",
                *result.modelEdge,
                "Theoretical value today minus the current market mark.",
                true
            );
        } else {
            DrawResultRow("Current mark", "Not provided");
        }

        if (result.maxLoss.has_value() && *result.maxLoss > 0.0) {
            const double risk = *result.maxLoss;
            DrawPercentRow(
                "Return on risk",
                (result.currentPL / risk) * 100.0,
                "Current theoretical P/L divided by maximum loss."
            );
        } else {
            DrawResultRow("Return on risk", "N/A", "Current theoretical P/L divided by maximum loss.");
        }

        DrawNumberRow("Delta", result.delta, "Estimated change in strategy value for a $1 underlying move.");
        DrawNumberRow("Theta / day", result.theta, "Estimated one-calendar-day time decay, using Black-Scholes theta.");

        ImGui::EndTable();
    }

    ImGui::Spacing();
    ImGui::TextUnformatted("Greeks");
    if (ImGui::BeginTable("LegGreeks", 4, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Leg", ImGuiTableColumnFlags_WidthFixed, 70.0f);
        ImGui::TableSetupColumn("Instrument", ImGuiTableColumnFlags_WidthFixed, 110.0f);
        ImGui::TableSetupColumn("Delta", ImGuiTableColumnFlags_WidthFixed, 110.0f);
        ImGui::TableSetupColumn("Theta / day", ImGuiTableColumnFlags_WidthFixed, 110.0f);
        ImGui::TableHeadersRow();

        for (int legIndex = 0; legIndex < static_cast<int>(result.evaluatedLegs.size()); ++legIndex) {
            const StrategyLeg& leg = result.evaluatedLegs[legIndex];
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Leg %d", legIndex + 1);
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(InstrumentDisplayName(leg));
            ImGui::TableNextColumn();
            ImGui::Text("%.2f", leg.delta);
            ImGui::TableNextColumn();
            ImGui::Text("%.2f", leg.theta);
        }

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Total");
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Strategy");
        ImGui::TableNextColumn();
        ImGui::Text("%.2f", result.delta);
        ImGui::TableNextColumn();
        ImGui::Text("%.2f", result.theta);

        ImGui::EndTable();
    }
}

void DrawStrategyPage(const StrategyTemplate& strategyTemplate) {
    StrategySession& session = SessionForTemplate(strategyTemplate);

    ImGui::TextUnformatted(session.strategy.name.c_str());
    ImGui::SameLine();
    if (ImGui::Button("Reset to Template Defaults")) {
        session.strategy = MakeStrategyInstance(strategyTemplate);
    }
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
