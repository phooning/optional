#include "options_calculator.hpp"

#include "sidebar.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <cmath>
#include <cstdio>
#include <imgui.h>
#include <implot.h>
#include <vector>

namespace options_calculator {
namespace {

constexpr double kTradingDaysPerYear = 365.0;
constexpr double kSqrtTwo = 1.4142135623730951;
constexpr double kLongCallPayoffAngleRadians = 0.5235987755982989;

double NormalCdf(double value) {
    return 0.5 * std::erfc(-value / kSqrtTwo);
}

double BlackScholesCall(double spot, double strike, double yearsToExpiry, double volatility, double riskFreeRate) {
    if (yearsToExpiry <= 0.0 || volatility <= 0.0 || spot <= 0.0 || strike <= 0.0) {
        return std::max(0.0, spot - strike);
    }

    const double sigmaSqrtT = volatility * std::sqrt(yearsToExpiry);
    const double d1 = (std::log(spot / strike) + (riskFreeRate + 0.5 * volatility * volatility) * yearsToExpiry) / sigmaSqrtT;
    const double d2 = d1 - sigmaSqrtT;

    return spot * NormalCdf(d1) - strike * std::exp(-riskFreeRate * yearsToExpiry) * NormalCdf(d2);
}

double LongCallExpirationProfit(double spot, double strike, double premium, double contractShares) {
    return (std::max(0.0, spot - strike) - premium) * contractShares;
}

double ClampDouble(double value, double minValue, double maxValue) {
    return std::min(std::max(value, minValue), maxValue);
}

double LongCallTheoreticalProfit(
    double spot,
    double strike,
    double premium,
    double contractShares,
    double daysToExpiry,
    double volatility,
    double riskFreeRate
) {
    const double yearsToExpiry = std::max(0.0, daysToExpiry) / kTradingDaysPerYear;
    return (BlackScholesCall(spot, strike, yearsToExpiry, volatility, riskFreeRate) - premium) * contractShares;
}

void DrawResultRow(const char* label, const char* value) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(label);
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(value);
}

void DrawCurrencyResultRow(const char* label, double value) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(label);
    ImGui::TableNextColumn();
    ImGui::Text("$%.2f", value);
}

void DrawLongCallCalculator() {
    static double underlyingPrice = 100.0;
    static double strikePrice = 105.0;
    static double premium = 3.50;
    static double impliedVolatilityPercent = 25.0;
    static double riskFreeRatePercent = 4.5;
    static int daysToExpiry = 30;
    static int contracts = 1;
    static int sharesPerContract = 100;

    ImGui::TextUnformatted("Long Call");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::SetNextItemWidth(160.0f);
    ImGui::InputDouble("Underlying price", &underlyingPrice, 0.50, 5.0, "%.2f");
    ImGui::SetNextItemWidth(160.0f);
    ImGui::InputDouble("Strike price", &strikePrice, 0.50, 5.0, "%.2f");
    ImGui::SetNextItemWidth(160.0f);
    ImGui::InputDouble("Premium paid", &premium, 0.05, 0.50, "%.2f");
    ImGui::SetNextItemWidth(160.0f);
    ImGui::InputDouble("Implied volatility %", &impliedVolatilityPercent, 0.25, 2.50, "%.2f");
    ImGui::SetNextItemWidth(160.0f);
    ImGui::InputDouble("Risk-free rate %", &riskFreeRatePercent, 0.10, 0.50, "%.2f");
    ImGui::SetNextItemWidth(160.0f);
    ImGui::InputInt("Days to expiration", &daysToExpiry);
    ImGui::SetNextItemWidth(160.0f);
    ImGui::InputInt("Contracts", &contracts);
    ImGui::SetNextItemWidth(160.0f);
    ImGui::InputInt("Shares per contract", &sharesPerContract);

    underlyingPrice = std::max(0.0, underlyingPrice);
    strikePrice = std::max(0.0, strikePrice);
    premium = std::max(0.0, premium);
    impliedVolatilityPercent = std::max(0.01, impliedVolatilityPercent);
    daysToExpiry = std::max(0, daysToExpiry);
    contracts = std::max(1, contracts);
    sharesPerContract = std::max(1, sharesPerContract);

    const double contractShares = static_cast<double>(contracts * sharesPerContract);
    const double totalPremium = premium * contractShares;
    const double breakeven = strikePrice + premium;
    const double volatility = impliedVolatilityPercent / 100.0;
    const double riskFreeRate = riskFreeRatePercent / 100.0;
    const double intrinsicValue = std::max(0.0, underlyingPrice - strikePrice) * contractShares;
    const double profitLoss = intrinsicValue - totalPremium;
    const double currentTheoreticalValue = BlackScholesCall(
        underlyingPrice,
        strikePrice,
        static_cast<double>(daysToExpiry) / kTradingDaysPerYear,
        volatility,
        riskFreeRate
    ) * contractShares;
    const double currentTheoreticalPL = LongCallTheoreticalProfit(
        underlyingPrice,
        strikePrice,
        premium,
        contractShares,
        static_cast<double>(daysToExpiry),
        volatility,
        riskFreeRate
    );
    const double theoreticalROI = totalPremium > 0.0 ? (currentTheoreticalPL / totalPremium) * 100.0 : 0.0;

    ImGui::Spacing();
    if (ImGui::BeginTable("LongCallResults", 2, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Metric", ImGuiTableColumnFlags_WidthFixed, 270.0f);
        ImGui::TableSetupColumn("Value");

        DrawCurrencyResultRow("Breakeven", breakeven);
        DrawCurrencyResultRow("Max loss", totalPremium);
        DrawResultRow("Max profit", "Unlimited");
        DrawCurrencyResultRow("Current intrinsic value (if expired now)", intrinsicValue);
        DrawCurrencyResultRow("Current intrinsic P/L (if expired now)", profitLoss);
        DrawCurrencyResultRow("Current theoretical value", currentTheoreticalValue);
        DrawCurrencyResultRow("Current theoretical P/L", currentTheoreticalPL);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Return on risk");
        ImGui::TableNextColumn();
        ImGui::Text("%.2f%% (theoretical)", theoreticalROI);

        ImGui::EndTable();
    }

    ImGui::Spacing();

    const double xMin = 0.0;
    const double xMax = std::max(1.0, strikePrice * 2.0);

    constexpr int kPointCount = 240;
    static std::vector<double> underlyingPrices(kPointCount);
    static std::vector<double> expirationProfit(kPointCount);
    static std::vector<double> todayProfit(kPointCount);
    static std::array<std::vector<double>, 4> futureProfit = {
        std::vector<double>(kPointCount),
        std::vector<double>(kPointCount),
        std::vector<double>(kPointCount),
        std::vector<double>(kPointCount),
    };
    constexpr std::array<int, 4> kForwardDays = {1, 7, 14, 30};
    std::vector<double> losingPayoffX;
    std::vector<double> losingPayoffY;
    std::vector<double> winningPayoffX;
    std::vector<double> winningPayoffY;
    losingPayoffX.reserve(kPointCount + 1);
    losingPayoffY.reserve(kPointCount + 1);
    winningPayoffX.reserve(kPointCount + 1);
    winningPayoffY.reserve(kPointCount + 1);

    for (int pointIndex = 0; pointIndex < kPointCount; ++pointIndex) {
        const double t = static_cast<double>(pointIndex) / static_cast<double>(kPointCount - 1);
        const double spot = xMin + (xMax - xMin) * t;
        underlyingPrices[pointIndex] = spot;
        expirationProfit[pointIndex] = LongCallExpirationProfit(spot, strikePrice, premium, contractShares);
        todayProfit[pointIndex] = LongCallTheoreticalProfit(
            spot,
            strikePrice,
            premium,
            contractShares,
            static_cast<double>(daysToExpiry),
            volatility,
            riskFreeRate
        );

        for (int curveIndex = 0; curveIndex < static_cast<int>(kForwardDays.size()); ++curveIndex) {
            futureProfit[curveIndex][pointIndex] = LongCallTheoreticalProfit(
                spot,
                strikePrice,
                premium,
                contractShares,
                static_cast<double>(std::max(0, daysToExpiry - kForwardDays[curveIndex])),
                volatility,
                riskFreeRate
            );
        }
    }

    for (int pointIndex = 0; pointIndex < kPointCount; ++pointIndex) {
        const double spot = underlyingPrices[pointIndex];
        const double payoff = expirationProfit[pointIndex];
        if (payoff <= 0.0) {
            losingPayoffX.push_back(spot);
            losingPayoffY.push_back(payoff);
        }
        if (payoff >= 0.0) {
            winningPayoffX.push_back(spot);
            winningPayoffY.push_back(payoff);
        }
    }

    if (breakeven > xMin && breakeven < xMax) {
        losingPayoffX.push_back(breakeven);
        losingPayoffY.push_back(0.0);
        winningPayoffX.insert(winningPayoffX.begin(), breakeven);
        winningPayoffY.insert(winningPayoffY.begin(), 0.0);
    }

    const ImVec2 requestedPlotSize = ImVec2(-1.0f, 1080.0f);
    const float plotWidthEstimate = std::max(1.0f, requestedPlotSize.x > 0.0f ? requestedPlotSize.x : ImGui::GetContentRegionAvail().x);
    const float plotHeightEstimate = std::max(1.0f, requestedPlotSize.y > 0.0f ? requestedPlotSize.y : 360.0f);

    static double linkedXMin = 0.0;
    static double linkedXMax = 0.0;
    static double linkedYMin = 0.0;
    static double linkedYMax = 0.0;
    static double previousTargetYRange = 0.0;
    static double previousStrikePrice = -1.0;
    static double previousPremium = -1.0;
    static int previousContracts = -1;
    static int previousSharesPerContract = -1;

    const bool resetPlotScale =
        linkedXMax <= linkedXMin ||
        std::abs(previousStrikePrice - strikePrice) > 0.001 ||
        std::abs(previousPremium - premium) > 0.001 ||
        previousContracts != contracts ||
        previousSharesPerContract != sharesPerContract;

    if (resetPlotScale) {
        linkedXMin = xMin;
        linkedXMax = xMax;
        previousStrikePrice = strikePrice;
        previousPremium = premium;
        previousContracts = contracts;
        previousSharesPerContract = sharesPerContract;
    }

    linkedXMin = ClampDouble(linkedXMin, xMin, xMax - 0.01);
    linkedXMax = ClampDouble(linkedXMax, linkedXMin + 0.01, xMax);

    const bool resetYScale = resetPlotScale || linkedYMax <= linkedYMin || previousTargetYRange <= 0.0;
    const double visibleXRange = std::max(0.01, linkedXMax - linkedXMin);
    const double targetYRange = contractShares * visibleXRange * static_cast<double>(plotHeightEstimate) /
        (static_cast<double>(plotWidthEstimate) * std::tan(kLongCallPayoffAngleRadians));
    const double visiblePayoffMin = std::min(
        LongCallExpirationProfit(linkedXMin, strikePrice, premium, contractShares),
        LongCallExpirationProfit(linkedXMax, strikePrice, premium, contractShares)
    );
    const double visiblePayoffMax = std::max(
        LongCallExpirationProfit(linkedXMin, strikePrice, premium, contractShares),
        LongCallExpirationProfit(linkedXMax, strikePrice, premium, contractShares)
    );
    const double visibleYMin = std::min(-totalPremium, visiblePayoffMin);
    const double visibleYMax = std::max(0.0, visiblePayoffMax);
    const double yPadding = targetYRange * 0.04;
    const double neededYRange = std::max(targetYRange, (visibleYMax - visibleYMin) + (yPadding * 2.0));

    if (resetYScale) {
        linkedYMin = visibleYMin - yPadding;
        linkedYMax = linkedYMin + neededYRange;
        if (linkedYMax < visibleYMax + yPadding) {
            linkedYMax = visibleYMax + yPadding;
            linkedYMin = linkedYMax - neededYRange;
        }
    } else {
        const double currentYCenter = (linkedYMin + linkedYMax) * 0.5;
        linkedYMin = currentYCenter - (neededYRange * 0.5);
        linkedYMax = currentYCenter + (neededYRange * 0.5);
    }
    previousTargetYRange = targetYRange;

    if (ImPlot::BeginPlot("Long Call P/L", requestedPlotSize, ImPlotFlags_Crosshairs)) {
        ImPlot::SetupAxes("Underlying Price", "Profit / Loss");
        ImPlot::SetupAxisLimitsConstraints(ImAxis_X1, xMin, xMax);
        ImPlot::SetupAxisLinks(ImAxis_X1, &linkedXMin, &linkedXMax);
        ImPlot::SetupAxisLinks(ImAxis_Y1, &linkedYMin, &linkedYMax);

        const ImVec4 lossColor = ImVec4(0.95f, 0.25f, 0.25f, 1.0f);
        const ImVec4 profitColor = ImVec4(0.25f, 0.78f, 0.42f, 1.0f);
        if (!losingPayoffX.empty()) {
            ImPlot::PlotShaded(
                "Loss area##LongCallLossArea",
                losingPayoffX.data(),
                losingPayoffY.data(),
                static_cast<int>(losingPayoffX.size()),
                0.0,
                {
                    ImPlotProp_FillColor, lossColor,
                    ImPlotProp_FillAlpha, 0.22f,
                    ImPlotProp_Flags, ImPlotItemFlags_NoLegend
                }
            );
            ImPlot::PlotLine(
                "Payoff at expiration (loss)",
                losingPayoffX.data(),
                losingPayoffY.data(),
                static_cast<int>(losingPayoffX.size()),
                {ImPlotProp_LineColor, lossColor, ImPlotProp_LineWeight, 2.5f}
            );
        }

        if (!winningPayoffX.empty()) {
            ImPlot::PlotShaded(
                "Profit area##LongCallProfitArea",
                winningPayoffX.data(),
                winningPayoffY.data(),
                static_cast<int>(winningPayoffX.size()),
                0.0,
                {
                    ImPlotProp_FillColor, profitColor,
                    ImPlotProp_FillAlpha, 0.22f,
                    ImPlotProp_Flags, ImPlotItemFlags_NoLegend
                }
            );
            ImPlot::PlotLine(
                "Payoff at expiration (profit)",
                winningPayoffX.data(),
                winningPayoffY.data(),
                static_cast<int>(winningPayoffX.size()),
                {ImPlotProp_LineColor, profitColor, ImPlotProp_LineWeight, 2.5f}
            );
        }

        ImPlot::HideNextItem(true, ImPlotCond_Once);
        ImPlot::PlotLine(
            "Theoretical P/L today",
            underlyingPrices.data(),
            todayProfit.data(),
            kPointCount,
            {ImPlotProp_LineColor, ImVec4(0.95f, 0.75f, 0.20f, 1.0f), ImPlotProp_LineWeight, 2.0f}
        );

        for (int curveIndex = 0; curveIndex < static_cast<int>(kForwardDays.size()); ++curveIndex) {
            char label[32];
            std::snprintf(label, sizeof(label), "Theoretical P/L +%d day%s", kForwardDays[curveIndex], kForwardDays[curveIndex] == 1 ? "" : "s");
            ImPlot::HideNextItem(true, ImPlotCond_Once);
            ImPlot::PlotLine(label, underlyingPrices.data(), futureProfit[curveIndex].data(), kPointCount);
        }

        const double spotLine = underlyingPrice;
        const double strikeLine = strikePrice;
        const double breakevenLine = breakeven;
        const double zeroProfitLine = 0.0;
        const double maxLossLine = -totalPremium;
        ImPlot::PlotInfLines("Current spot", &spotLine, 1);
        ImPlot::PlotInfLines("Strike", &strikeLine, 1);
        ImPlot::PlotInfLines("Breakeven", &breakevenLine, 1);
        ImPlot::PlotInfLines("Breakeven P/L", &zeroProfitLine, 1, {ImPlotProp_Flags, ImPlotInfLinesFlags_Horizontal});
        ImPlot::PlotInfLines("Max loss", &maxLossLine, 1, {ImPlotProp_Flags, ImPlotInfLinesFlags_Horizontal});

        const double currentSpotY = LongCallExpirationProfit(underlyingPrice, strikePrice, premium, contractShares);
        const double breakevenY = 0.0;
        const double strikeY = LongCallExpirationProfit(strikePrice, strikePrice, premium, contractShares);
        const double displayedMaxProfitX = xMax;
        const double displayedMaxProfitY = expirationProfit.back();

        ImPlot::PlotScatter("Current spot marker", &underlyingPrice, &currentSpotY, 1, {ImPlotProp_Marker, ImPlotMarker_Circle, ImPlotProp_MarkerSize, 7.0f});
        ImPlot::PlotScatter("Breakeven marker", &breakevenLine, &breakevenY, 1, {ImPlotProp_Marker, ImPlotMarker_Diamond, ImPlotProp_MarkerSize, 7.0f});
        ImPlot::PlotScatter("Strike marker", &strikeLine, &strikeY, 1, {ImPlotProp_Marker, ImPlotMarker_Square, ImPlotProp_MarkerSize, 7.0f});
        ImPlot::PlotScatter("Max profit shown", &displayedMaxProfitX, &displayedMaxProfitY, 1, {ImPlotProp_Marker, ImPlotMarker_Up, ImPlotProp_MarkerSize, 8.0f});

        if (ImPlot::IsPlotHovered()) {
            const ImPlotPoint mousePosition = ImPlot::GetPlotMousePos();
            const double snappedX = ClampDouble(mousePosition.x, xMin, xMax);
            const double snappedY = LongCallExpirationProfit(snappedX, strikePrice, premium, contractShares);
            const ImVec4 cursorColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

            ImPlot::PlotInfLines(
                "Cursor snap##LongCallCursorLine",
                &snappedX,
                1,
                {
                    ImPlotProp_LineColor, cursorColor,
                    ImPlotProp_LineWeight, 1.5f,
                    ImPlotProp_Flags, ImPlotItemFlags_NoLegend
                }
            );
            ImPlot::PlotScatter(
                "Cursor snap##LongCallCursorPoint",
                &snappedX,
                &snappedY,
                1,
                {
                    ImPlotProp_Marker, ImPlotMarker_Circle,
                    ImPlotProp_MarkerSize, 8.0f,
                    ImPlotProp_MarkerFillColor, cursorColor,
                    ImPlotProp_MarkerLineColor, cursorColor,
                    ImPlotProp_Flags, ImPlotItemFlags_NoLegend
                }
            );
            ImPlot::Annotation(
                snappedX,
                snappedY,
                cursorColor,
                ImVec2(10.0f, -22.0f),
                true,
                "Spot $%.2f\nExpiration P/L $%.2f",
                snappedX,
                snappedY
            );
            ImGui::SetTooltip("Payoff at expiration\nUnderlying: $%.2f\nP/L: $%.2f", snappedX, snappedY);
        }

        ImPlot::EndPlot();
    }

    ImGui::TextDisabled("Max profit is unlimited; the plotted marker shows the highest payoff inside the current x-axis range.");
}

void DrawEmptyStrategyPage() {
}

} // namespace

void Draw(ImGuiID dockspaceId) {
    static const char* selectedStrategy = DefaultStrategyName();

    if (dockspaceId != 0) ImGui::SetNextWindowDockID(dockspaceId, ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Options Calculator")) {
        const float sidebarWidth = 245.0f;
        ImGui::BeginChild("StrategySidebar", ImVec2(sidebarWidth, 0.0f), true);
        DrawStrategySidebar(&selectedStrategy);
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("StrategyPage", ImVec2(0.0f, 0.0f), false);
        if (std::strcmp(selectedStrategy, "Long Call") == 0) {
            DrawLongCallCalculator();
        } else {
            DrawEmptyStrategyPage();
        }
        ImGui::EndChild();
    }

    ImGui::End();
}

} // namespace options_calculator
