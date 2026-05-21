#include "utils.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace options_calculator {

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

} // namespace options_calculator
