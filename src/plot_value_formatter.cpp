// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "rviz_2d_plot_plugin/plot_value_formatter.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace rviz_2d_plot_plugin
{
namespace
{

constexpr double kZeroEpsilon = 1e-10;

double normalizedZero(const double value, const double scale)
{
  if (std::abs(value) <= std::max(scale, 1.0) * kZeroEpsilon) {
    return 0.0;
  }
  return value;
}

std::string trimTrailingZeros(std::string text)
{
  const std::string::size_type exponent = text.find_first_of("eE");
  std::string suffix;
  if (exponent != std::string::npos) {
    suffix = text.substr(exponent);
    text.erase(exponent);
  }

  const std::string::size_type decimal = text.find('.');
  if (decimal != std::string::npos) {
    while (!text.empty() && text.back() == '0') {
      text.pop_back();
    }
    if (!text.empty() && text.back() == '.') {
      text.pop_back();
    }
  }

  if (text == "-0") {
    text = "0";
  }
  return text + suffix;
}

std::string fixedValue(const double value, const int decimals)
{
  std::ostringstream stream;
  stream << std::fixed << std::setprecision(decimals) << value;
  return trimTrailingZeros(stream.str());
}

}  // namespace

std::string formatPlotValue(double value)
{
  value = normalizedZero(value, 1.0);

  std::ostringstream stream;
  stream << std::setprecision(4) << value;
  return trimTrailingZeros(stream.str());
}

std::string formatAxisTickValue(double value, const double step)
{
  value = normalizedZero(value, step);
  if (step > 0.0 && step < 1.0) {
    const int decimals = std::clamp(
      static_cast<int>(std::ceil(-std::log10(step))) + 1,
      1,
      6);
    return fixedValue(value, decimals);
  }
  return formatPlotValue(value);
}

}  // namespace rviz_2d_plot_plugin
