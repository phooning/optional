#pragma once

#include <string>

namespace options_calculator {

double ClampDouble(double value, double minValue, double maxValue);
std::string FormatCurrency(double value);

} // namespace options_calculator
