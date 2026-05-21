#include "payoff_plot.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <imgui.h>
#include <implot.h>
#include <string>

namespace options_calculator {
namespace {

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

double InterpolateCurve(const PayoffCurve& curve, double spot) {
    if (curve.spots.empty() || curve.values.empty() || curve.spots.size() != curve.values.size()) {
        return 0.0;
    }

    if (spot <= curve.spots.front()) {
        return curve.values.front();
    }

    if (spot >= curve.spots.back()) {
        return curve.values.back();
    }

    const auto upper = std::upper_bound(curve.spots.begin(), curve.spots.end(), spot);
    const std::size_t index = static_cast<std::size_t>(std::distance(curve.spots.begin(), upper));
    const double leftX = curve.spots[index - 1];
    const double rightX = curve.spots[index];
    const double leftY = curve.values[index - 1];
    const double rightY = curve.values[index];
    const double t = (spot - leftX) / std::max(0.000001, rightX - leftX);
    return leftY + (rightY - leftY) * t;
}

double CurveMinimum(const PayoffCurve& curve) {
    if (curve.values.empty()) {
        return 0.0;
    }

    return *std::min_element(curve.values.begin(), curve.values.end());
}

double CurveMaximum(const PayoffCurve& curve) {
    if (curve.values.empty()) {
        return 0.0;
    }

    return *std::max_element(curve.values.begin(), curve.values.end());
}

} // namespace

void DrawPayoffPlot(const StrategyInstance& strategy, const MarketInputs& market, const StrategyResult& result) {
    if (result.expirationCurve.spots.empty() || result.expirationCurve.values.empty()) {
        return;
    }

    const double xMin = result.expirationCurve.spots.front();
    const double xMax = result.expirationCurve.spots.back();
    double yMin = std::min({0.0, CurveMinimum(result.expirationCurve), CurveMinimum(result.theoreticalCurveToday)});
    double yMax = std::max({0.0, CurveMaximum(result.expirationCurve), CurveMaximum(result.theoreticalCurveToday)});
    for (const PayoffCurve& curve : result.futureCurves) {
        yMin = std::min(yMin, CurveMinimum(curve));
        yMax = std::max(yMax, CurveMaximum(curve));
    }

    const double yRange = std::max(1.0, yMax - yMin);
    yMin -= yRange * 0.08;
    yMax += yRange * 0.08;

    static double linkedXMin = 0.0;
    static double linkedXMax = 0.0;
    static double linkedYMin = 0.0;
    static double linkedYMax = 0.0;
    static std::string previousStrategyName;

    if (previousStrategyName != strategy.name || linkedXMax <= linkedXMin || linkedYMax <= linkedYMin) {
        linkedXMin = xMin;
        linkedXMax = xMax;
        linkedYMin = yMin;
        linkedYMax = yMax;
        previousStrategyName = strategy.name;
    }

    linkedXMin = ClampDouble(linkedXMin, xMin, xMax - 0.01);
    linkedXMax = ClampDouble(linkedXMax, linkedXMin + 0.01, xMax);

    const ImVec2 requestedPlotSize = ImVec2(-1.0f, 720.0f);
    if (ImPlot::BeginPlot("Strategy P/L", requestedPlotSize, ImPlotFlags_Crosshairs)) {
        ImPlot::SetupAxes("Underlying Price", "Profit / Loss");
        ImPlot::SetupAxisLimitsConstraints(ImAxis_X1, xMin, xMax);
        ImPlot::SetupAxisLinks(ImAxis_X1, &linkedXMin, &linkedXMax);
        ImPlot::SetupAxisLinks(ImAxis_Y1, &linkedYMin, &linkedYMax);

        ImPlot::PlotLine(
            result.expirationCurve.label.c_str(),
            result.expirationCurve.spots.data(),
            result.expirationCurve.values.data(),
            static_cast<int>(result.expirationCurve.spots.size()),
            {ImPlotProp_LineColor, ImVec4(0.25f, 0.78f, 0.42f, 1.0f), ImPlotProp_LineWeight, 2.5f}
        );

        if (!result.theoreticalCurveToday.spots.empty()) {
            ImPlot::HideNextItem(true, ImPlotCond_Once);
            ImPlot::PlotLine(
                result.theoreticalCurveToday.label.c_str(),
                result.theoreticalCurveToday.spots.data(),
                result.theoreticalCurveToday.values.data(),
                static_cast<int>(result.theoreticalCurveToday.spots.size()),
                {ImPlotProp_LineColor, ImVec4(0.95f, 0.75f, 0.20f, 1.0f), ImPlotProp_LineWeight, 2.0f}
            );
        }

        for (const PayoffCurve& curve : result.futureCurves) {
            ImPlot::HideNextItem(true, ImPlotCond_Once);
            ImPlot::PlotLine(
                curve.label.c_str(),
                curve.spots.data(),
                curve.values.data(),
                static_cast<int>(curve.spots.size())
            );
        }

        const double zeroProfitLine = 0.0;
        ImPlot::PlotInfLines("Zero P/L", &zeroProfitLine, 1, {ImPlotProp_Flags, ImPlotInfLinesFlags_Horizontal});

        const double currentSpot = ClampDouble(market.underlyingPrice, xMin, xMax);
        ImPlot::PlotInfLines("Current spot", &currentSpot, 1);
        const double currentSpotY = InterpolateCurve(result.expirationCurve, currentSpot);
        ImPlot::PlotScatter(
            "Current spot marker",
            &currentSpot,
            &currentSpotY,
            1,
            {ImPlotProp_Marker, ImPlotMarker_Circle, ImPlotProp_MarkerSize, 7.0f}
        );

        for (int breakevenIndex = 0; breakevenIndex < static_cast<int>(result.breakevens.size()); ++breakevenIndex) {
            const double breakeven = result.breakevens[breakevenIndex];
            if (breakeven <= xMin || breakeven >= xMax) {
                continue;
            }

            char lineLabel[64];
            std::snprintf(lineLabel, sizeof(lineLabel), "Breakeven %d", breakevenIndex + 1);
            ImPlot::PlotInfLines(lineLabel, &breakeven, 1);

            const double breakevenY = 0.0;
            char markerLabel[64];
            std::snprintf(markerLabel, sizeof(markerLabel), "Breakeven marker %d", breakevenIndex + 1);
            ImPlot::PlotScatter(
                markerLabel,
                &breakeven,
                &breakevenY,
                1,
                {ImPlotProp_Marker, ImPlotMarker_Diamond, ImPlotProp_MarkerSize, 7.0f}
            );
        }

        if (ImPlot::IsPlotHovered()) {
            const ImPlotPoint mousePosition = ImPlot::GetPlotMousePos();
            const double snappedX = ClampDouble(mousePosition.x, xMin, xMax);
            const double snappedY = InterpolateCurve(result.expirationCurve, snappedX);
            const std::string snappedXText = FormatCurrency(snappedX);
            const std::string snappedYText = FormatCurrency(snappedY);
            const ImVec4 cursorColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

            ImPlot::PlotInfLines(
                "Cursor snap##StrategyCursorLine",
                &snappedX,
                1,
                {
                    ImPlotProp_LineColor, cursorColor,
                    ImPlotProp_LineWeight, 1.5f,
                    ImPlotProp_Flags, ImPlotItemFlags_NoLegend
                }
            );
            ImPlot::PlotScatter(
                "Cursor snap##StrategyCursorPoint",
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
                "Spot %s\nExpiration P/L %s",
                snappedXText.c_str(),
                snappedYText.c_str()
            );
            ImGui::SetTooltip("Payoff at expiration\nUnderlying: %s\nP/L: %s", snappedXText.c_str(), snappedYText.c_str());
        }

        ImPlot::EndPlot();
    }

    if (!result.maxProfit.has_value()) {
        ImGui::TextDisabled("Max profit is unlimited; the plotted curve shows payoff inside the current x-axis range.");
    }
    if (!result.maxLoss.has_value()) {
        ImGui::TextDisabled("Max loss is unlimited; the plotted curve shows payoff inside the current x-axis range.");
    }
}

} // namespace options_calculator
